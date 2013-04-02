/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef MOZ_VALGRIND
# include <valgrind/helgrind.h>
# include <valgrind/memcheck.h>
#else
# define VALGRIND_HG_MUTEX_LOCK_PRE(_mx,_istry)  /* */
# define VALGRIND_HG_MUTEX_LOCK_POST(_mx)        /* */
# define VALGRIND_HG_MUTEX_UNLOCK_PRE(_mx)       /* */
# define VALGRIND_HG_MUTEX_UNLOCK_POST(_mx)      /* */
# define VALGRIND_MAKE_MEM_DEFINED(_addr,_len)   ((void)0)
# define VALGRIND_MAKE_MEM_UNDEFINED(_addr,_len) ((void)0)
#endif

#include "mozilla/arm.h"
#include "mozilla/StandardInteger.h"
#include "PlatformMacros.h"

#include "sps_sampler.h"
#include "platform.h"
#include <iostream>

#include "ProfileEntry2.h"
#include "UnwinderThread2.h"

#if !defined(SPS_OS_windows)
# include <sys/time.h>
# include <unistd.h>
# include <pthread.h>
  // mmap
# include <sys/mman.h>
#endif

#if defined(SPS_OS_android)
# include "android-signal-defs.h"
#endif

#if defined(SPS_OS_darwin)
# include <mach-o/dyld.h>
#endif


/* Verbosity of this module, for debugging:
     0  silent
     1  adds info about debuginfo load success/failure
     2  adds slow-summary stats for buffer fills/misses (RECOMMENDED)
     3  adds per-sample summary lines
     4  adds per-sample frame listing
   Note that level 3 and above produces risk of deadlock, and 
   are not recommended for extended use.
*/
#define LOGLEVEL 2


// The 'else' of this covers the entire rest of the file
#if defined(SPS_OS_windows)

//////////////////////////////////////////////////////////
//// BEGIN externally visible functions (WINDOWS STUBS)

// On Windows this will all need reworking.  sps_sampler.h
// will ensure these functions are never actually called,
// so just provide no-op stubs for now.

void uwt__init()
{
}

void uwt__deinit()
{
}

void uwt__register_thread_for_profiling ( void* stackTop )
{
}

// RUNS IN SIGHANDLER CONTEXT
UnwinderThreadBuffer* uwt__acquire_empty_buffer()
{
  return NULL;
}

// RUNS IN SIGHANDLER CONTEXT
void
uwt__release_full_buffer(ThreadProfile2* aProfile,
                         UnwinderThreadBuffer* utb,
                         void* /* ucontext_t*, really */ ucV )
{
}

// RUNS IN SIGHANDLER CONTEXT
void
utb__addEntry(/*MODIFIED*/UnwinderThreadBuffer* utb, ProfileEntry2 ent)
{
}

//// END externally visible functions (WINDOWS STUBS)
//////////////////////////////////////////////////////////

#else // a supported target

//////////////////////////////////////////////////////////
//// BEGIN externally visible functions

// Forward references
// the unwinder thread ID, its fn, and a stop-now flag
static void* unwind_thr_fn ( void* exit_nowV );
static pthread_t unwind_thr;
static int       unwind_thr_exit_now = 0; // RACED ON

// Threads must be registered with this file before they can be
// sampled.  So that we know the max safe stack address for each
// registered thread.
static void thread_register_for_profiling ( void* stackTop );

// RUNS IN SIGHANDLER CONTEXT
// Acquire an empty buffer and mark it as FILLING
static UnwinderThreadBuffer* acquire_empty_buffer();

// RUNS IN SIGHANDLER CONTEXT
// Put this buffer in the queue of stuff going to the unwinder
// thread, and mark it as FULL.  Before doing that, fill in stack
// chunk and register fields if a native unwind is requested.
// APROFILE is where the profile data should be added to.  UTB
// is the partially-filled-in buffer, containing ProfileEntries.
// UCV is the ucontext_t* from the signal handler.  If non-NULL, is
// taken as a cue to request native unwind.
static void release_full_buffer(ThreadProfile2* aProfile,
                                UnwinderThreadBuffer* utb,
                                void* /* ucontext_t*, really */ ucV );

// RUNS IN SIGHANDLER CONTEXT
static void utb_add_prof_ent(UnwinderThreadBuffer* utb, ProfileEntry2 ent);

// Do a store memory barrier.
static void do_MBAR();


void uwt__init()
{
  // Create the unwinder thread.
  MOZ_ASSERT(unwind_thr_exit_now == 0);
  int r = pthread_create( &unwind_thr, NULL,
                          unwind_thr_fn, (void*)&unwind_thr_exit_now );
  MOZ_ALWAYS_TRUE(r==0);
}

void uwt__deinit()
{
  // Shut down the unwinder thread.
  MOZ_ASSERT(unwind_thr_exit_now == 0);
  unwind_thr_exit_now = 1;
  do_MBAR();
  int r = pthread_join(unwind_thr, NULL); MOZ_ALWAYS_TRUE(r==0);
}

void uwt__register_thread_for_profiling(void* stackTop)
{
  thread_register_for_profiling(stackTop);
}

// RUNS IN SIGHANDLER CONTEXT
UnwinderThreadBuffer* uwt__acquire_empty_buffer()
{
  return acquire_empty_buffer();
}

// RUNS IN SIGHANDLER CONTEXT
void
uwt__release_full_buffer(ThreadProfile2* aProfile,
                         UnwinderThreadBuffer* utb,
                         void* /* ucontext_t*, really */ ucV )
{
  release_full_buffer( aProfile, utb, ucV );
}

// RUNS IN SIGHANDLER CONTEXT
void
utb__addEntry(/*MODIFIED*/UnwinderThreadBuffer* utb, ProfileEntry2 ent)
{
  utb_add_prof_ent(utb, ent);
}

//// END externally visible functions
//////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////
//// BEGIN type UnwindThreadBuffer

MOZ_STATIC_ASSERT(sizeof(uint32_t) == 4, "uint32_t size incorrect");
MOZ_STATIC_ASSERT(sizeof(uint64_t) == 8, "uint64_t size incorrect");
MOZ_STATIC_ASSERT(sizeof(uintptr_t) == sizeof(void*),
                  "uintptr_t size incorrect");

typedef
  struct { 
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rip; 
  }
  AMD64Regs;

typedef
  struct {
    uint32_t r15;
    uint32_t r14;
    uint32_t r13;
    uint32_t r12;
    uint32_t r11;
    uint32_t r7;
  }
  ARMRegs;

typedef
  struct {
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
  }
  X86Regs;

#if defined(SPS_ARCH_amd64)
typedef  AMD64Regs  ArchRegs;
#elif defined(SPS_ARCH_arm)
typedef  ARMRegs  ArchRegs;
#elif defined(SPS_ARCH_x86)
typedef  X86Regs  ArchRegs;
#else
# error "Unknown plat"
#endif

#if defined(SPS_ARCH_amd64) || defined(SPS_ARCH_arm) || defined(SPS_ARCH_x86)
# define SPS_PAGE_SIZE 4096
#else
# error "Unknown plat"
#endif

typedef  enum { S_EMPTY, S_FILLING, S_EMPTYING, S_FULL }  State;

typedef  struct { uintptr_t val; }  SpinLock;

/* CONFIGURABLE */
/* The maximum number of bytes in a stack snapshot */
#define N_STACK_BYTES 32768

/* CONFIGURABLE */
/* The number of fixed ProfileEntry2 slots.  If more are required, they
   are placed in mmap'd pages. */
#define N_FIXED_PROF_ENTS 20

/* CONFIGURABLE */
/* The number of extra pages of ProfileEntries.  If (on arm) each
   ProfileEntry2 is 8 bytes, then a page holds 512, and so 100 pages
   is enough to hold 51200. */
#define N_PROF_ENT_PAGES 100

/* DERIVATIVE */
#define N_PROF_ENTS_PER_PAGE (SPS_PAGE_SIZE / sizeof(ProfileEntry2))

/* A page of ProfileEntry2s.  This might actually be slightly smaller
   than a page if SPS_PAGE_SIZE is not an exact multiple of
   sizeof(ProfileEntry2). */
typedef
  struct { ProfileEntry2 ents[N_PROF_ENTS_PER_PAGE]; }
  ProfEntsPage;

#define ProfEntsPage_INVALID ((ProfEntsPage*)1)


/* Fields protected by the spinlock are marked SL */

struct _UnwinderThreadBuffer {
  /*SL*/ State  state;
  /* The rest of these are protected, in some sense, by ::state.  If
     ::state is S_FILLING, they are 'owned' by the sampler thread
     that set the state to S_FILLING.  If ::state is S_EMPTYING,
     they are 'owned' by the unwinder thread that set the state to
     S_EMPTYING.  If ::state is S_EMPTY or S_FULL, the buffer isn't
     owned by any thread, and so no thread may access these
     fields. */
  /* Sample number, needed to process samples in order */
  uint64_t       seqNo;
  /* The ThreadProfile2 into which the results are eventually to be
     dumped. */
  ThreadProfile2* aProfile;
  /* Pseudostack and other info, always present */
  ProfileEntry2   entsFixed[N_FIXED_PROF_ENTS];
  ProfEntsPage*  entsPages[N_PROF_ENT_PAGES];
  uintptr_t      entsUsed;
  /* Do we also have data to do a native unwind? */
  bool           haveNativeInfo;
  /* If so, here is the register state and stack.  Unset if
     .haveNativeInfo is false. */
  ArchRegs       regs;
  unsigned char  stackImg[N_STACK_BYTES];
  unsigned int   stackImgUsed;
  void*          stackImgAddr; /* VMA corresponding to stackImg[0] */
  void*          stackMaxSafe; /* VMA for max safe stack reading */
};
/* Indexing scheme for ents:
     0 <= i < N_FIXED_PROF_ENTS
       is at entsFixed[i]

     i >= N_FIXED_PROF_ENTS
       is at let j = i - N_FIXED_PROF_ENTS
             in  entsPages[j / N_PROFENTS_PER_PAGE]
                  ->ents[j % N_PROFENTS_PER_PAGE]
     
   entsPages[] are allocated on demand.  Because zero can
   theoretically be a valid page pointer, use 
   ProfEntsPage_INVALID == (ProfEntsPage*)1 to mark invalid pages.

   It follows that the max entsUsed value is N_FIXED_PROF_ENTS +
   N_PROFENTS_PER_PAGE * N_PROFENTS_PAGES, and at that point no more
   ProfileEntries can be storedd.
*/


typedef
  struct {
    pthread_t thrId;
    void*     stackTop;
    uint64_t  nSamples; 
  }
  StackLimit;

/* Globals -- the buffer array */
#define N_UNW_THR_BUFFERS 10
/*SL*/ static UnwinderThreadBuffer** g_buffers     = NULL;
/*SL*/ static uint64_t               g_seqNo       = 0;
/*SL*/ static SpinLock               g_spinLock    = { 0 };

/* Globals -- the thread array */
#define N_SAMPLING_THREADS 10
/*SL*/ static StackLimit g_stackLimits[N_SAMPLING_THREADS];
/*SL*/ static int        g_stackLimitsUsed = 0;

/* Stats -- atomically incremented, no lock needed */
static uintptr_t g_stats_totalSamples = 0; // total # sample attempts
static uintptr_t g_stats_noBuffAvail  = 0; // # failed due to no buffer avail

/* We must be VERY CAREFUL what we do with the spinlock held.  The
   only thing it is safe to do with it held is modify (viz, read or
   write) g_buffers, g_buffers[], g_seqNo, g_buffers[]->state,
   g_stackLimits[] and g_stackLimitsUsed.  No arbitrary computations,
   no syscalls, no printfs, no file IO, and absolutely no dynamic
   memory allocation (else we WILL eventually deadlock).

   This applies both to the signal handler and to the unwinder thread.
*/

//// END type UnwindThreadBuffer
//////////////////////////////////////////////////////////

// fwds
// the interface to breakpad
typedef  struct { u_int64_t pc; u_int64_t sp; }  PCandSP;

static
void do_breakpad_unwind_Buffer(/*OUT*/PCandSP** pairs,
                               /*OUT*/unsigned int* nPairs,
                               UnwinderThreadBuffer* buff,
                               int buffNo /* for debug printing only */);

static bool is_page_aligned(void* v)
{
  uintptr_t w = (uintptr_t) v;
  return (w & (SPS_PAGE_SIZE-1)) == 0  ? true  : false;
}


/* Implement machine-word sized atomic compare-and-swap.  Returns true
   if success, false if failure. */
static bool do_CASW(uintptr_t* addr, uintptr_t expected, uintptr_t nyu)
{
#if defined(__GNUC__)
  return __sync_bool_compare_and_swap(addr, expected, nyu);
#else
# error "Unhandled compiler"
#endif
}

/* Hint to the CPU core that we are in a spin-wait loop, and that
   other processors/cores/threads-running-on-the-same-core should be
   given priority on execute resources, if that is possible.  Not
   critical if this is a no-op on some targets. */
static void do_SPINLOOP_RELAX()
{
#if (defined(SPS_ARCH_amd64) || defined(SPS_ARCH_x86)) && defined(__GNUC__)
  __asm__ __volatile__("rep; nop");
#elif defined(SPS_PLAT_arm_android) && MOZILLA_ARM_ARCH >= 7
  __asm__ __volatile__("wfe");
#endif
}

/* Tell any cores snoozing in spin loops to wake up. */
static void do_SPINLOOP_NUDGE()
{
#if (defined(SPS_ARCH_amd64) || defined(SPS_ARCH_x86)) && defined(__GNUC__)
  /* this is a no-op */
#elif defined(SPS_PLAT_arm_android) && MOZILLA_ARM_ARCH >= 7
  __asm__ __volatile__("sev");
#endif
}

/* Perform a full memory barrier. */
static void do_MBAR()
{
#if defined(__GNUC__)
  __sync_synchronize();
#else
# error "Unhandled compiler"
#endif
}

static void spinLock_acquire(SpinLock* sl)
{
  uintptr_t* val = &sl->val;
  VALGRIND_HG_MUTEX_LOCK_PRE(sl, 0/*!isTryLock*/);
  while (1) {
    bool ok = do_CASW( val, 0, 1 );
    if (ok) break;
    do_SPINLOOP_RELAX();
  }
  do_MBAR();
  VALGRIND_HG_MUTEX_LOCK_POST(sl);
}

static void spinLock_release(SpinLock* sl)
{
  uintptr_t* val = &sl->val;
  VALGRIND_HG_MUTEX_UNLOCK_PRE(sl);
  do_MBAR();
  bool ok = do_CASW( val, 1, 0 );
  /* This must succeed at the first try.  To fail would imply that
     the lock was unheld. */
  MOZ_ALWAYS_TRUE(ok);
  do_SPINLOOP_NUDGE();
  VALGRIND_HG_MUTEX_UNLOCK_POST(sl);
}

static void sleep_ms(unsigned int ms)
{
  struct timespec req;
  req.tv_sec = ((time_t)ms) / 1000;
  req.tv_nsec = 1000 * 1000 * (((unsigned long)ms) % 1000);
  nanosleep(&req, NULL);
}

/* Use CAS to implement standalone atomic increment. */
static void atomic_INC(uintptr_t* loc)
{
  while (1) {
    uintptr_t old = *loc;
    uintptr_t nyu = old + 1;
    bool ok = do_CASW( loc, old, nyu );
    if (ok) break;
  }
}

/* Register a thread for profiling.  It must not be allowed to receive
   signals before this is done, else the signal handler will
   MOZ_ASSERT. */
static void thread_register_for_profiling(void* stackTop)
{
  int i;
  /* Minimal sanity check on stackTop */
  MOZ_ASSERT( (void*)&i < stackTop );

  spinLock_acquire(&g_spinLock);

  pthread_t me = pthread_self();
  for (i = 0; i < g_stackLimitsUsed; i++) {
    /* check for duplicate registration */
    MOZ_ASSERT(g_stackLimits[i].thrId != me);
  }
  if (!(g_stackLimitsUsed < N_SAMPLING_THREADS))
    MOZ_CRASH();  // Don't continue -- we'll get memory corruption.
  g_stackLimits[g_stackLimitsUsed].thrId    = me;
  g_stackLimits[g_stackLimitsUsed].stackTop = stackTop;
  g_stackLimits[g_stackLimitsUsed].nSamples = 0;
  g_stackLimitsUsed++;

  spinLock_release(&g_spinLock);
  LOGF("BPUnw: thread_register_for_profiling(stacktop %p, me %p)", 
       stackTop, (void*)me);
}


__attribute__((unused))
static void show_registered_threads()
{
  int i;
  spinLock_acquire(&g_spinLock);
  for (i = 0; i < g_stackLimitsUsed; i++) {
    LOGF("[%d]  pthread_t=%p  nSamples=%lld",
         i, (void*)g_stackLimits[i].thrId, 
            (unsigned long long int)g_stackLimits[i].nSamples);
  }
  spinLock_release(&g_spinLock);
}


// RUNS IN SIGHANDLER CONTEXT
static UnwinderThreadBuffer* acquire_empty_buffer()
{
  /* acq lock
     if buffers == NULL { rel lock; exit }
     scan to find a free buff; if none { rel lock; exit }
     set buff state to S_FILLING
     fillseqno++; and remember it
     rel lock
  */
  int i;

  atomic_INC( &g_stats_totalSamples );

  /* This code is critical.  We are in a signal handler and possibly
     with the malloc lock held.  So we can't allocate any heap, and
     can't safely call any C library functions, not even the pthread_
     functions.  And we certainly can't do any syscalls.  In short,
     this function needs to be self contained, not do any allocation,
     and not hold on to the spinlock for any significant length of
     time. */

  spinLock_acquire(&g_spinLock);

  /* First of all, look for this thread's entry in g_stackLimits[].
     We need to find it in order to figure out how much stack we can
     safely copy into the sample.  This assumes that pthread_self()
     is safe to call in a signal handler, which strikes me as highly
     likely. */
  pthread_t me = pthread_self();
  MOZ_ASSERT(g_stackLimitsUsed >= 0 && g_stackLimitsUsed <= N_SAMPLING_THREADS);
  for (i = 0; i < g_stackLimitsUsed; i++) {
    if (g_stackLimits[i].thrId == me)
      break;
  }
  /* "this thread is registered for profiling" */
  MOZ_ASSERT(i < g_stackLimitsUsed);

  /* The furthest point that we can safely scan back up the stack. */
  void* myStackTop = g_stackLimits[i].stackTop;
  g_stackLimits[i].nSamples++;

  /* Try to find a free buffer to use. */
  if (g_buffers == NULL) {
    /* The unwinder thread hasn't allocated any buffers yet.
       Nothing we can do. */
    spinLock_release(&g_spinLock);
    atomic_INC( &g_stats_noBuffAvail );
    return NULL;
  }

  for (i = 0; i < N_UNW_THR_BUFFERS; i++) {
    if (g_buffers[i]->state == S_EMPTY)
      break;
  }
  MOZ_ASSERT(i >= 0 && i <= N_UNW_THR_BUFFERS);

  if (i == N_UNW_THR_BUFFERS) {
    /* Again, no free buffers .. give up. */
    spinLock_release(&g_spinLock);
    atomic_INC( &g_stats_noBuffAvail );
    if (LOGLEVEL >= 3)
      LOG("BPUnw: handler:  no free buffers");
    return NULL;
  }

  /* So we can use this one safely.  Whilst still holding the lock,
     mark the buffer as belonging to us, and increment the sequence
     number. */
  UnwinderThreadBuffer* buff = g_buffers[i];
  MOZ_ASSERT(buff->state == S_EMPTY);
  buff->state = S_FILLING;
  buff->seqNo = g_seqNo;
  g_seqNo++;

  /* And drop the lock.  We own the buffer, so go on and fill it. */
  spinLock_release(&g_spinLock);

  /* Now we own the buffer, initialise it. */
  buff->aProfile       = NULL;
  buff->entsUsed       = 0;
  buff->haveNativeInfo = false;
  buff->stackImgUsed   = 0;
  buff->stackImgAddr   = 0;
  buff->stackMaxSafe   = myStackTop; /* We will need this in
                                        release_full_buffer() */
  for (i = 0; i < N_PROF_ENT_PAGES; i++)
    buff->entsPages[i] = ProfEntsPage_INVALID;
  return buff;
}


// RUNS IN SIGHANDLER CONTEXT
/* The calling thread owns the buffer, as denoted by its state being
   S_FILLING.  So we can mess with it without further locking. */
static void release_full_buffer(ThreadProfile2* aProfile,
                                UnwinderThreadBuffer* buff,
                                void* /* ucontext_t*, really */ ucV )
{
  MOZ_ASSERT(buff->state == S_FILLING);

  ////////////////////////////////////////////////////
  // BEGIN fill

  /* The buffer already will have some of its ProfileEntries filled
     in, but everything else needs to be filled in at this point. */
  //LOGF("Release full buffer: %lu ents", buff->entsUsed);
  /* Where the resulting info is to be dumped */
  buff->aProfile = aProfile;

  /* And, if we have register state, that and the stack top */
  buff->haveNativeInfo = ucV != NULL;
  if (buff->haveNativeInfo) {
#   if defined(SPS_PLAT_amd64_linux)
    ucontext_t* uc = (ucontext_t*)ucV;
    mcontext_t* mc = &(uc->uc_mcontext);
    buff->regs.rip = mc->gregs[REG_RIP];
    buff->regs.rsp = mc->gregs[REG_RSP];
    buff->regs.rbp = mc->gregs[REG_RBP];
#   elif defined(SPS_PLAT_amd64_darwin)
    ucontext_t* uc = (ucontext_t*)ucV;
    struct __darwin_mcontext64* mc = uc->uc_mcontext;
    struct __darwin_x86_thread_state64* ss = &mc->__ss;
    buff->regs.rip = ss->__rip;
    buff->regs.rsp = ss->__rsp;
    buff->regs.rbp = ss->__rbp;
#   elif defined(SPS_PLAT_arm_android)
    ucontext_t* uc = (ucontext_t*)ucV;
    mcontext_t* mc = &(uc->uc_mcontext);
    buff->regs.r15 = mc->arm_pc; //gregs[R15];
    buff->regs.r14 = mc->arm_lr; //gregs[R14];
    buff->regs.r13 = mc->arm_sp; //gregs[R13];
    buff->regs.r12 = mc->arm_ip; //gregs[R12];
    buff->regs.r11 = mc->arm_fp; //gregs[R11];
    buff->regs.r7  = mc->arm_r7; //gregs[R7];
#   elif defined(SPS_PLAT_x86_linux)
    ucontext_t* uc = (ucontext_t*)ucV;
    mcontext_t* mc = &(uc->uc_mcontext);
    buff->regs.eip = mc->gregs[REG_EIP];
    buff->regs.esp = mc->gregs[REG_ESP];
    buff->regs.ebp = mc->gregs[REG_EBP];
#   elif defined(SPS_PLAT_x86_darwin)
    ucontext_t* uc = (ucontext_t*)ucV;
    struct __darwin_mcontext32* mc = uc->uc_mcontext;
    struct __darwin_i386_thread_state* ss = &mc->__ss;
    buff->regs.eip = ss->__eip;
    buff->regs.esp = ss->__esp;
    buff->regs.ebp = ss->__ebp;
#   elif defined(SPS_PLAT_x86_android)
    ucontext_t* uc = (ucontext_t*)ucV;
    mcontext_t* mc = &(uc->uc_mcontext);
    buff->regs.eip = mc->eip;
    buff->regs.esp = mc->esp;
    buff->regs.ebp = mc->ebp;
#   else
#     error "Unknown plat"
#   endif

    /* Copy up to N_STACK_BYTES from rsp-REDZONE upwards, but not
       going past the stack's registered top point.  Do some basic
       sanity checks too. */
    { 
#     if defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_amd64_darwin)
      uintptr_t rEDZONE_SIZE = 128;
      uintptr_t start = buff->regs.rsp - rEDZONE_SIZE;
#     elif defined(SPS_PLAT_arm_android)
      uintptr_t rEDZONE_SIZE = 0;
      uintptr_t start = buff->regs.r13 - rEDZONE_SIZE;
#     elif defined(SPS_PLAT_x86_linux) || defined(SPS_PLAT_x86_darwin) \
           || defined(SPS_PLAT_x86_android)
      uintptr_t rEDZONE_SIZE = 0;
      uintptr_t start = buff->regs.esp - rEDZONE_SIZE;
#     else
#       error "Unknown plat"
#     endif
      uintptr_t end   = (uintptr_t)buff->stackMaxSafe;
      uintptr_t ws    = sizeof(void*);
      start &= ~(ws-1);
      end   &= ~(ws-1);
      uintptr_t nToCopy = 0;
      if (start < end) {
        nToCopy = end - start;
        if (nToCopy > N_STACK_BYTES)
          nToCopy = N_STACK_BYTES;
      }
      MOZ_ASSERT(nToCopy <= N_STACK_BYTES);
      buff->stackImgUsed = nToCopy;
      buff->stackImgAddr = (void*)start;
      if (nToCopy > 0) {
        memcpy(&buff->stackImg[0], (void*)start, nToCopy);
        (void)VALGRIND_MAKE_MEM_DEFINED(&buff->stackImg[0], nToCopy);
      }
    }
  } /* if (buff->haveNativeInfo) */
  // END fill
  ////////////////////////////////////////////////////

  /* And now relinquish ownership of the buff, so that an unwinder
     thread can pick it up. */
  spinLock_acquire(&g_spinLock);
  buff->state = S_FULL;
  spinLock_release(&g_spinLock);
}


// RUNS IN SIGHANDLER CONTEXT
// Allocate a ProfEntsPage, without using malloc, or return
// ProfEntsPage_INVALID if we can't for some reason.
static ProfEntsPage* mmap_anon_ProfEntsPage()
{
# if defined(SPS_OS_darwin)
  void* v = ::mmap(NULL, sizeof(ProfEntsPage), PROT_READ|PROT_WRITE, 
                   MAP_PRIVATE|MAP_ANON,      -1, 0);
# else
  void* v = ::mmap(NULL, sizeof(ProfEntsPage), PROT_READ|PROT_WRITE, 
                   MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
# endif
  if (v == MAP_FAILED) {
    return ProfEntsPage_INVALID;
  } else {
    return (ProfEntsPage*)v;
  }
}

// Runs in the unwinder thread
// Free a ProfEntsPage as allocated by mmap_anon_ProfEntsPage
static void munmap_ProfEntsPage(ProfEntsPage* pep)
{
  MOZ_ALWAYS_TRUE(is_page_aligned(pep));
  ::munmap(pep, sizeof(ProfEntsPage));
}


// RUNS IN SIGHANDLER CONTEXT
void
utb_add_prof_ent(/*MODIFIED*/UnwinderThreadBuffer* utb, ProfileEntry2 ent)
{
  uintptr_t limit
    = N_FIXED_PROF_ENTS + (N_PROF_ENTS_PER_PAGE * N_PROF_ENT_PAGES);
  if (utb->entsUsed == limit) {
    /* We're full.  Now what? */
    LOG("BPUnw: utb__addEntry: NO SPACE for ProfileEntry2; ignoring.");
    return;
  }
  MOZ_ASSERT(utb->entsUsed < limit);

  /* Will it fit in the fixed array? */
  if (utb->entsUsed < N_FIXED_PROF_ENTS) {
    utb->entsFixed[utb->entsUsed] = ent;
    utb->entsUsed++;
    return;
  }

  /* No.  Put it in the extras. */
  uintptr_t i     = utb->entsUsed;
  uintptr_t j     = i - N_FIXED_PROF_ENTS;
  uintptr_t j_div = j / N_PROF_ENTS_PER_PAGE; /* page number */
  uintptr_t j_mod = j % N_PROF_ENTS_PER_PAGE; /* page offset */
  ProfEntsPage* pep = utb->entsPages[j_div];
  if (pep == ProfEntsPage_INVALID) {
    pep = mmap_anon_ProfEntsPage();
    if (pep == ProfEntsPage_INVALID) {
      /* Urr, we ran out of memory.  Now what? */
      LOG("BPUnw: utb__addEntry: MMAP FAILED for ProfileEntry2; ignoring.");
      return;
    }
    utb->entsPages[j_div] = pep;
  }
  pep->ents[j_mod] = ent;
  utb->entsUsed++;
}


// misc helper
static ProfileEntry2 utb_get_profent(UnwinderThreadBuffer* buff, uintptr_t i)
{
  MOZ_ASSERT(i < buff->entsUsed);
  if (i < N_FIXED_PROF_ENTS) {
    return buff->entsFixed[i];
  } else {
    uintptr_t j     = i - N_FIXED_PROF_ENTS;
    uintptr_t j_div = j / N_PROF_ENTS_PER_PAGE; /* page number */
    uintptr_t j_mod = j % N_PROF_ENTS_PER_PAGE; /* page offset */
    MOZ_ASSERT(buff->entsPages[j_div] != ProfEntsPage_INVALID);
    return buff->entsPages[j_div]->ents[j_mod];
  }
}


// Runs in the unwinder thread -- well, this _is_ the unwinder thread.
static void* unwind_thr_fn(void* exit_nowV)
{
  /* If we're the first thread in, we'll need to allocate the buffer
     array g_buffers plus the Buffer structs that it points at. */
  spinLock_acquire(&g_spinLock);
  if (g_buffers == NULL) {
    /* Drop the lock, make a complete copy in memory, reacquire the
       lock, and try to install it -- which might fail, if someone
       else beat us to it. */
    spinLock_release(&g_spinLock);
    UnwinderThreadBuffer** buffers
      = (UnwinderThreadBuffer**)malloc(N_UNW_THR_BUFFERS
                                        * sizeof(UnwinderThreadBuffer*));
    MOZ_ASSERT(buffers);
    int i;
    for (i = 0; i < N_UNW_THR_BUFFERS; i++) {
      buffers[i] = (UnwinderThreadBuffer*)
                   calloc(sizeof(UnwinderThreadBuffer), 1);
      MOZ_ASSERT(buffers[i]);
      buffers[i]->state = S_EMPTY;
    }
    /* Try to install it */
    spinLock_acquire(&g_spinLock);
    if (g_buffers == NULL) {
      g_buffers = buffers;
      spinLock_release(&g_spinLock);
    } else {
      /* Someone else beat us to it.  Release what we just allocated
         so as to avoid a leak. */
      spinLock_release(&g_spinLock);
      for (i = 0; i < N_UNW_THR_BUFFERS; i++) {
        free(buffers[i]);
      }
      free(buffers);
    }
  } else {
    /* They are already allocated, so just drop the lock and continue. */
    spinLock_release(&g_spinLock);
  }

  /* 
    while (1) {
      acq lock
      scan to find oldest full
         if none { rel lock; sleep; continue }
      set buff state to emptying
      rel lock
      acq MLock // implicitly
      process buffer
      rel MLock // implicitly
      acq lock
      set buff state to S_EMPTY
      rel lock
    }
  */
  int* exit_now = (int*)exit_nowV;
  int ms_to_sleep_if_empty = 1;
  while (1) {

    if (*exit_now != 0) break;

    spinLock_acquire(&g_spinLock);

    /* Find the oldest filled buffer, if any. */
    uint64_t oldest_seqNo = ~0ULL; /* infinity */
    int      oldest_ix    = -1;
    int      i;
    for (i = 0; i < N_UNW_THR_BUFFERS; i++) {
      UnwinderThreadBuffer* buff = g_buffers[i];
      if (buff->state != S_FULL) continue;
      if (buff->seqNo < oldest_seqNo) {
        oldest_seqNo = buff->seqNo;
        oldest_ix    = i;
      }
    }
    if (oldest_ix == -1) {
      /* We didn't find a full buffer.  Snooze and try again later. */
      MOZ_ASSERT(oldest_seqNo == ~0ULL);
      spinLock_release(&g_spinLock);
      if (ms_to_sleep_if_empty > 100 && LOGLEVEL >= 2) {
        LOGF("BPUnw: unwinder: sleep for %d ms", ms_to_sleep_if_empty);
      }
      sleep_ms(ms_to_sleep_if_empty);
      if (ms_to_sleep_if_empty < 20) {
        ms_to_sleep_if_empty += 2;
      } else {
        ms_to_sleep_if_empty = (15 * ms_to_sleep_if_empty) / 10;
        if (ms_to_sleep_if_empty > 1000)
          ms_to_sleep_if_empty = 1000;
      }
      continue;
    }

    /* We found a full a buffer.  Mark it as 'ours' and drop the
       lock; then we can safely throw breakpad at it. */
    UnwinderThreadBuffer* buff = g_buffers[oldest_ix];
    MOZ_ASSERT(buff->state == S_FULL);
    buff->state = S_EMPTYING;
    spinLock_release(&g_spinLock);

    /* unwind .. in which we can do anything we like, since any
       resource stalls that we may encounter (eg malloc locks) in
       competition with signal handler instances, will be short
       lived since the signal handler is guaranteed nonblocking. */
    if (0) LOGF("BPUnw: unwinder: seqNo %llu: emptying buf %d\n",
                (unsigned long long int)oldest_seqNo, oldest_ix);

    /* Copy ProfileEntries presented to us by the sampling thread.
       Most of them are copied verbatim into |buff->aProfile|,
       except for 'hint' tags, which direct us to do something
       different. */

    /* Need to lock |aProfile| so nobody tries to copy out entries
       whilst we are putting them in. */
    buff->aProfile->GetMutex()->Lock();

    /* The buff is a sequence of ProfileEntries (ents).  It has
       this grammar:

       | --pre-tags-- | (h 'P' .. h 'Q')* | --post-tags-- |
                        ^               ^
                        ix_first_hP     ix_last_hQ

       Each (h 'P' .. h 'Q') subsequence represents one pseudostack
       entry.  These, if present, are in the order
       outermost-frame-first, and that is the order that they should
       be copied into aProfile.  The --pre-tags-- and --post-tags--
       are to be copied into the aProfile verbatim, except that they
       may contain the hints "h 'F'" for a flush and "h 'N'" to
       indicate that a native unwind is also required, and must be
       interleaved with the pseudostack entries.

       The hint tags that bound each pseudostack entry, "h 'P'" and "h
       'Q'", are not to be copied into the aProfile -- they are
       present only to make parsing easy here.  Also, the pseudostack
       entries may contain an "'S' (void*)" entry, which is the stack
       pointer value for that entry, and these are also not to be
       copied.
    */
    /* The first thing to do is therefore to find the pseudostack
       entries, if any, and to find out also whether a native unwind
       has been requested. */
    const uintptr_t infUW = ~(uintptr_t)0; // infinity
    bool  need_native_unw = false;
    uintptr_t ix_first_hP = infUW; // "not found"
    uintptr_t ix_last_hQ  = infUW; // "not found"

    uintptr_t k;
    for (k = 0; k < buff->entsUsed; k++) {
      ProfileEntry2 ent = utb_get_profent(buff, k);
      if (ent.is_ent_hint('N')) {
        need_native_unw = true;
      }
      else if (ent.is_ent_hint('P') && ix_first_hP == ~(uintptr_t)0) {
        ix_first_hP = k;
      }
      else if (ent.is_ent_hint('Q')) {
        ix_last_hQ = k;
      }
    }

    if (0) LOGF("BPUnw: ix_first_hP %llu  ix_last_hQ %llu  need_native_unw %llu",
                (unsigned long long int)ix_first_hP,
                (unsigned long long int)ix_last_hQ,
                (unsigned long long int)need_native_unw);

    /* There are four possibilities: native-only, pseudostack-only,
       combined (both), and neither.  We handle all four cases. */

    MOZ_ASSERT( (ix_first_hP == infUW && ix_last_hQ == infUW) ||
                (ix_first_hP != infUW && ix_last_hQ != infUW) );
    bool have_P = ix_first_hP != infUW;
    if (have_P) {
      MOZ_ASSERT(ix_first_hP < ix_last_hQ);
      MOZ_ASSERT(ix_last_hQ <= buff->entsUsed);
    }

    /* Neither N nor P.  This is very unusual but has been observed to happen.
       Just copy to the output. */
    if (!need_native_unw && !have_P) {
      for (k = 0; k < buff->entsUsed; k++) {
        ProfileEntry2 ent = utb_get_profent(buff, k);
        // action flush-hints
        if (ent.is_ent_hint('F')) { buff->aProfile->flush(); continue; }
        // skip ones we can't copy
        if (ent.is_ent_hint() || ent.is_ent('S')) { continue; }
        // and copy everything else
        buff->aProfile->addTag( ent );
      }
    }
    else /* Native only-case. */
    if (need_native_unw && !have_P) {
      for (k = 0; k < buff->entsUsed; k++) {
        ProfileEntry2 ent = utb_get_profent(buff, k);
        // action a native-unwind-now hint
        if (ent.is_ent_hint('N')) {
          MOZ_ASSERT(buff->haveNativeInfo);
          PCandSP* pairs = NULL;
          unsigned int nPairs = 0;
          do_breakpad_unwind_Buffer(&pairs, &nPairs, buff, oldest_ix);
          buff->aProfile->addTag( ProfileEntry2('s', "(root)") );
          for (unsigned int i = 0; i < nPairs; i++) {
            buff->aProfile
                ->addTag( ProfileEntry2('l', reinterpret_cast<void*>(pairs[i].pc)) );
          }
          if (pairs)
            free(pairs);
          continue;
        }
        // action flush-hints
        if (ent.is_ent_hint('F')) { buff->aProfile->flush(); continue; }
        // skip ones we can't copy
        if (ent.is_ent_hint() || ent.is_ent('S')) { continue; }
        // and copy everything else
        buff->aProfile->addTag( ent );
      }
    }
    else /* Pseudostack-only case */
    if (!need_native_unw && have_P) {
      /* If there's no request for a native stack, it's easy: just
         copy the tags verbatim into aProfile, skipping the ones that
         can't be copied -- 'h' (hint) tags, and "'S' (void*)"
         stack-pointer tags.  Except, insert a sample-start tag when
         we see the start of the first pseudostack frame. */
      for (k = 0; k < buff->entsUsed; k++) {
        ProfileEntry2 ent = utb_get_profent(buff, k);
        // We need to insert a sample-start tag before the first frame
        if (k == ix_first_hP) {
          buff->aProfile->addTag( ProfileEntry2('s', "(root)") );
        }
        // action flush-hints
        if (ent.is_ent_hint('F')) { buff->aProfile->flush(); continue; }
        // skip ones we can't copy
        if (ent.is_ent_hint() || ent.is_ent('S')) { continue; }
        // and copy everything else
        buff->aProfile->addTag( ent );
      }
    }
    else /* Combined case */
    if (need_native_unw && have_P)
    {
      /* We need to get a native stacktrace and merge it with the
         pseudostack entries.  This isn't too simple.  First, copy all
         the tags up to the start of the pseudostack tags.  Then
         generate a combined set of tags by native unwind and
         pseudostack.  Then, copy all the stuff after the pseudostack
         tags. */
      MOZ_ASSERT(buff->haveNativeInfo);

      // Get native unwind info
      PCandSP* pairs = NULL;
      unsigned int n_pairs = 0;
      do_breakpad_unwind_Buffer(&pairs, &n_pairs, buff, oldest_ix);

      // Entries before the pseudostack frames
      for (k = 0; k < ix_first_hP; k++) {
        ProfileEntry2 ent = utb_get_profent(buff, k);
        // action flush-hints
        if (ent.is_ent_hint('F')) { buff->aProfile->flush(); continue; }
        // skip ones we can't copy
        if (ent.is_ent_hint() || ent.is_ent('S')) { continue; }
        // and copy everything else
        buff->aProfile->addTag( ent );
      }

      // BEGIN merge
      buff->aProfile->addTag( ProfileEntry2('s', "(root)") );
      unsigned int next_N = 0; // index in pairs[]
      unsigned int next_P = ix_first_hP; // index in buff profent array
      bool last_was_P = false;
      if (0) LOGF("at mergeloop: n_pairs %llu ix_last_hQ %llu",
                  (unsigned long long int)n_pairs,
                  (unsigned long long int)ix_last_hQ);
      while (true) {
        if (next_P <= ix_last_hQ) {
          // Assert that next_P points at the start of an P entry
          MOZ_ASSERT(utb_get_profent(buff, next_P).is_ent_hint('P'));
        }
        if (next_N >= n_pairs && next_P > ix_last_hQ) {
          // both stacks empty
          break;
        }
        /* Decide which entry to use next:
           If N is empty, must use P, and vice versa
           else
           If the last was P and current P has zero SP, use P
           else
           we assume that both P and N have valid SP, in which case
              use the one with the larger value
        */
        bool use_P = true;
        if (next_N >= n_pairs) {
          // N empty, use P
          use_P = true;
          if (0) LOG("  P  <=  no remaining N entries");
        }
        else if (next_P > ix_last_hQ) {
          // P empty, use N
          use_P = false;
          if (0) LOG("  N  <=  no remaining P entries");
        }
        else {
          // We have at least one N and one P entry available.
          // Scan forwards to find the SP of the current P entry
          u_int64_t sp_cur_P = 0;
          unsigned int m = next_P + 1;
          while (1) {
            /* This assertion should hold because in a well formed
               input, we must eventually find the hint-Q that marks
               the end of this frame's entries. */
            MOZ_ASSERT(m < buff->entsUsed);
            ProfileEntry2 ent = utb_get_profent(buff, m);
            if (ent.is_ent_hint('Q'))
              break;
            if (ent.is_ent('S')) {
              sp_cur_P = reinterpret_cast<u_int64_t>(ent.get_tagPtr());
              break;
            }
            m++;
          }
          if (last_was_P && sp_cur_P == 0) {
            if (0) LOG("  P  <=  last_was_P && sp_cur_P == 0");
            use_P = true;
          } else {
            u_int64_t sp_cur_N = pairs[next_N].sp;
            use_P = (sp_cur_P > sp_cur_N);
            if (0) LOGF("  %s  <=  sps P %p N %p",
                        use_P ? "P" : "N", (void*)(intptr_t)sp_cur_P, 
                                           (void*)(intptr_t)sp_cur_N);
          }
        }
        /* So, we know which we are going to use. */
        if (use_P) {
          unsigned int m = next_P + 1;
          while (true) {
            MOZ_ASSERT(m < buff->entsUsed);
            ProfileEntry2 ent = utb_get_profent(buff, m);
            if (ent.is_ent_hint('Q')) {
              next_P = m + 1;
              break;
            }
            // we don't expect a flush-hint here
            MOZ_ASSERT(!ent.is_ent_hint('F'));
            // skip ones we can't copy
            if (ent.is_ent_hint() || ent.is_ent('S')) { m++; continue; }
            // and copy everything else
            buff->aProfile->addTag( ent );
            m++;
          }
        } else {
          buff->aProfile
              ->addTag( ProfileEntry2('l', reinterpret_cast<void*>(pairs[next_N].pc)) );
          next_N++;
        }
        /* Remember what we chose, for next time. */
        last_was_P = use_P;
      }

      MOZ_ASSERT(next_P == ix_last_hQ + 1);
      MOZ_ASSERT(next_N == n_pairs);
      // END merge

      // Entries after the pseudostack frames
      for (k = ix_last_hQ+1; k < buff->entsUsed; k++) {
        ProfileEntry2 ent = utb_get_profent(buff, k);
        // action flush-hints
        if (ent.is_ent_hint('F')) { buff->aProfile->flush(); continue; }
        // skip ones we can't copy
        if (ent.is_ent_hint() || ent.is_ent('S')) { continue; }
        // and copy everything else
        buff->aProfile->addTag( ent );
      }

      // free native unwind info
      if (pairs)
        free(pairs);
    }

#if 0
    bool show = true;
    if (show) LOG("----------------");
    for (k = 0; k < buff->entsUsed; k++) {
      ProfileEntry2 ent = utb_get_profent(buff, k);
      if (show) ent.log();
      if (ent.is_ent_hint('F')) {
        /* This is a flush-hint */
        buff->aProfile->flush();
      } 
      else if (ent.is_ent_hint('N')) {
        /* This is a do-a-native-unwind-right-now hint */
        MOZ_ASSERT(buff->haveNativeInfo);
        PCandSP* pairs = NULL;
        unsigned int nPairs = 0;
        do_breakpad_unwind_Buffer(&pairs, &nPairs, buff, oldest_ix);
        buff->aProfile->addTag( ProfileEntry2('s', "(root)") );
        for (unsigned int i = 0; i < nPairs; i++) {
          buff->aProfile
              ->addTag( ProfileEntry2('l', reinterpret_cast<void*>(pairs[i].pc)) );
        }
        if (pairs)
          free(pairs);
      } else {
        /* Copy in verbatim */
        buff->aProfile->addTag( ent );
      }
    }
#endif

    buff->aProfile->GetMutex()->Unlock();

    /* And .. we're done.  Mark the buffer as empty so it can be
       reused.  First though, unmap any of the entsPages that got
       mapped during filling. */
    for (i = 0; i < N_PROF_ENT_PAGES; i++) {
      if (buff->entsPages[i] == ProfEntsPage_INVALID)
        continue;
      munmap_ProfEntsPage(buff->entsPages[i]);
      buff->entsPages[i] = ProfEntsPage_INVALID;
    }

    (void)VALGRIND_MAKE_MEM_UNDEFINED(&buff->stackImg[0], N_STACK_BYTES);
    spinLock_acquire(&g_spinLock);
    MOZ_ASSERT(buff->state == S_EMPTYING);
    buff->state = S_EMPTY;
    spinLock_release(&g_spinLock);
    ms_to_sleep_if_empty = 1;
  }
  return NULL;
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

/* After this point, we have some classes that interface with
   breakpad, that allow us to pass in a Buffer and get an unwind of
   it. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "local_debug_info_symbolizer.h"
#include "processor/stackwalker_amd64.h"
#include "processor/stackwalker_arm.h"
#include "processor/stackwalker_x86.h"
#include "processor/logging.h"
#include "common/linux/dump_symbols.h"

#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/code_modules.h"

google_breakpad::MemoryRegion* foo = NULL;

using std::string;

///////////////////////////////////////////////////////////////////
/* Implement MemoryRegion, so that it hauls stack image data out of
   the stack top snapshots that the signal handler has so carefully
   snarfed. */

// BEGIN: DERIVED FROM src/processor/stackwalker_selftest.cc
//
class BufferMemoryRegion : public google_breakpad::MemoryRegion {
 public:
  // We just keep hold of the Buffer* we're given, but make no attempt
  // to take allocation-ownership of it.
  BufferMemoryRegion(UnwinderThreadBuffer* buff) : buff_(buff) { }
  ~BufferMemoryRegion() { }

  u_int64_t GetBase() const { return (uintptr_t)buff_->stackImgAddr; }
  u_int32_t GetSize() const { return (uintptr_t)buff_->stackImgUsed; }

  bool GetMemoryAtAddress(u_int64_t address, u_int8_t*  value) const {
      return GetMemoryAtAddressInternal(address, value); }
  bool GetMemoryAtAddress(u_int64_t address, u_int16_t* value) const {
      return GetMemoryAtAddressInternal(address, value); }
  bool GetMemoryAtAddress(u_int64_t address, u_int32_t* value) const {
      return GetMemoryAtAddressInternal(address, value); }
  bool GetMemoryAtAddress(u_int64_t address, u_int64_t* value) const {
      return GetMemoryAtAddressInternal(address, value); }

 private:
  template<typename T> bool GetMemoryAtAddressInternal (
                               u_int64_t address, T* value) const {
    /* Range check .. */
    if ( buff_->stackImgUsed >= sizeof(T)
         && ((uintptr_t)address) >= ((uintptr_t)buff_->stackImgAddr)
         && ((uintptr_t)address) <= ((uintptr_t)buff_->stackImgAddr)
                                     + buff_->stackImgUsed
                                     - sizeof(T) ) {
      uintptr_t offset = (uintptr_t)address - (uintptr_t)buff_->stackImgAddr;
      if (0) LOGF("GMAA %llx ok", (unsigned long long int)address);
      *value = *reinterpret_cast<const T*>(&buff_->stackImg[offset]);
      return true;
    } else {
      if (0) LOGF("GMAA %llx failed", (unsigned long long int)address);
      return false;
    }
  }

  // where this all comes from
  UnwinderThreadBuffer* buff_;
};
//
// END: DERIVED FROM src/processor/stackwalker_selftest.cc


///////////////////////////////////////////////////////////////////
/* Implement MyCodeModule and MyCodeModules, so they pull the relevant
   information about which modules are loaded where out of
   /proc/self/maps. */

class MyCodeModule : public google_breakpad::CodeModule {
public:
  MyCodeModule(u_int64_t x_start, u_int64_t x_len, string filename)
    : x_start_(x_start), x_len_(x_len), filename_(filename) {
    MOZ_ASSERT(x_len > 0);
  }

  ~MyCodeModule() {}

  // The base address of this code module as it was loaded by the process.
  // (u_int64_t)-1 on error.
  u_int64_t base_address() const { return x_start_; }

  // The size of the code module.  0 on error.
  u_int64_t size() const { return x_len_; }

  // The path or file name that the code module was loaded from.  Empty on
  // error.
  string code_file() const { return filename_; }

  // An identifying string used to discriminate between multiple versions and
  // builds of the same code module.  This may contain a uuid, timestamp,
  // version number, or any combination of this or other information, in an
  // implementation-defined format.  Empty on error.
  string code_identifier() const { MOZ_CRASH(); return ""; }

  // The filename containing debugging information associated with the code
  // module.  If debugging information is stored in a file separate from the
  // code module itself (as is the case when .pdb or .dSYM files are used),
  // this will be different from code_file.  If debugging information is
  // stored in the code module itself (possibly prior to stripping), this
  // will be the same as code_file.  Empty on error.
  string debug_file() const { MOZ_CRASH(); return ""; }

  // An identifying string similar to code_identifier, but identifies a
  // specific version and build of the associated debug file.  This may be
  // the same as code_identifier when the debug_file and code_file are
  // identical or when the same identifier is used to identify distinct
  // debug and code files.
  string debug_identifier() const { MOZ_CRASH(); return ""; }

  // A human-readable representation of the code module's version.  Empty on
  // error.
  string version() const { MOZ_CRASH(); return ""; }

  // Creates a new copy of this CodeModule object, which the caller takes
  // ownership of.  The new CodeModule may be of a different concrete class
  // than the CodeModule being copied, but will behave identically to the
  // copied CodeModule as far as the CodeModule interface is concerned.
  const CodeModule* Copy() const { MOZ_CRASH(); return NULL; }

 private:
  // record info for a file backed executable mapping
  // snarfed from /proc/self/maps
  u_int64_t x_start_;
  u_int64_t x_len_;    // may not be zero
  string    filename_; // of the mapped file
};


/* Find out, in a platform-dependent way, where the code modules got
   mapped in the process' virtual address space, and add them to
   |mods_|. */
static void read_procmaps(std::vector<MyCodeModule*>& mods_)
{
  MOZ_ASSERT(mods_.size() == 0);

#if defined(SPS_OS_linux) || defined(SPS_OS_android)
  // read /proc/self/maps and create a vector of CodeModule*
  FILE* f = fopen("/proc/self/maps", "r");
  MOZ_ASSERT(f);
  while (!feof(f)) {
    unsigned long long int start = 0;
    unsigned long long int end   = 0;
    char rr = ' ', ww = ' ', xx = ' ', pp = ' ';
    unsigned long long int offset = 0, inode = 0;
    unsigned int devMaj = 0, devMin = 0;
    int nItems = fscanf(f, "%llx-%llx %c%c%c%c %llx %x:%x %llu",
                        &start, &end, &rr, &ww, &xx, &pp,
                        &offset, &devMaj, &devMin, &inode);
    if (nItems == EOF && feof(f)) break;
    MOZ_ASSERT(nItems == 10);
    MOZ_ASSERT(start < end);
    // read the associated file name, if it is present
    int ch;
    // find '/' or EOL
    while (1) {
      ch = fgetc(f);
      MOZ_ASSERT(ch != EOF);
      if (ch == '\n' || ch == '/') break;
    }
    string fname("");
    if (ch == '/') {
      fname += (char)ch;
      while (1) {
        ch = fgetc(f);
        MOZ_ASSERT(ch != EOF);
        if (ch == '\n') break;
        fname += (char)ch;
      }
    }
    MOZ_ASSERT(ch == '\n');
    if (0) LOGF("SEG %llx %llx %c %c %c %c %s",
                start, end, rr, ww, xx, pp, fname.c_str() );
    if (xx == 'x' && fname != "") {
      MyCodeModule* cm = new MyCodeModule( start, end-start, fname );
      mods_.push_back(cm);
    }
  }
  fclose(f);

#elif defined(SPS_OS_darwin)

# if defined(SPS_PLAT_amd64_darwin)
  typedef mach_header_64 breakpad_mach_header;
  typedef segment_command_64 breakpad_mach_segment_command;
  const int LC_SEGMENT_XX = LC_SEGMENT_64;
# else // SPS_PLAT_x86_darwin
  typedef mach_header breakpad_mach_header;
  typedef segment_command breakpad_mach_segment_command;
  const int LC_SEGMENT_XX = LC_SEGMENT;
# endif

  uint32_t n_images = _dyld_image_count();
  for (uint32_t ix = 0; ix < n_images; ix++) {

    MyCodeModule* cm = NULL;
    unsigned long slide = _dyld_get_image_vmaddr_slide(ix);
    const char* name = _dyld_get_image_name(ix);
    const breakpad_mach_header* header
      = (breakpad_mach_header*)_dyld_get_image_header(ix);
    if (!header)
      continue;

    const struct load_command *cmd =
      reinterpret_cast<const struct load_command *>(header + 1);

    /* Look through the MachO headers to find out the module's stated
       VMA, so we can add it to the slide to find its actual VMA.
       Copied from MinidumpGenerator::WriteModuleStream
       src/client/mac/handler/minidump_generator.cc. */
    for (unsigned int i = 0; cmd && (i < header->ncmds); i++) {
      if (cmd->cmd == LC_SEGMENT_XX) {

        const breakpad_mach_segment_command *seg =
          reinterpret_cast<const breakpad_mach_segment_command *>(cmd);

        if (!strcmp(seg->segname, "__TEXT")) {
          cm = new MyCodeModule( seg->vmaddr + slide,
                                 seg->vmsize, string(name) );
          break;
        }
      }
      cmd = reinterpret_cast<struct load_command*>((char *)cmd + cmd->cmdsize);
    }
    if (cm) {
      mods_.push_back(cm);
      if (0) LOGF("SEG %llx %llx %s",
                  cm->base_address(), cm->base_address() + cm->size(),
                  cm->code_file().c_str());
    }
  }

#else
# error "Unknown platform"
#endif

  if (0) LOGF("got %d mappings\n", (int)mods_.size());
}


class MyCodeModules : public google_breakpad::CodeModules
{
 public:
  MyCodeModules() {
    read_procmaps(mods_);
  }

  ~MyCodeModules() {
    std::vector<MyCodeModule*>::const_iterator it;
    for (it = mods_.begin(); it < mods_.end(); it++) {
      MyCodeModule* cm = *it;
      delete cm;
    }
  }

 private:
  std::vector<MyCodeModule*> mods_;

  unsigned int module_count() const { MOZ_CRASH(); return 1; }

  const google_breakpad::CodeModule*
                GetModuleForAddress(u_int64_t address) const
  {
    if (0) printf("GMFA %llx\n", (unsigned long long int)address);
    std::vector<MyCodeModule*>::const_iterator it;
    for (it = mods_.begin(); it < mods_.end(); it++) {
       MyCodeModule* cm = *it;
       if (0) printf("considering %p  %llx +%llx\n",
                     (void*)cm, (unsigned long long int)cm->base_address(),
                                (unsigned long long int)cm->size());
       if (cm->base_address() <= address
           && address < cm->base_address() + cm->size())
          return cm;
    }
    return NULL;
  }

  const google_breakpad::CodeModule* GetMainModule() const {
    MOZ_CRASH(); return NULL; return NULL;
  }

  const google_breakpad::CodeModule* GetModuleAtSequence(
                unsigned int sequence) const {
    MOZ_CRASH(); return NULL;
  }

  const google_breakpad::CodeModule* GetModuleAtIndex(unsigned int index) const {
    MOZ_CRASH(); return NULL;
  }

  const CodeModules* Copy() const {
    MOZ_CRASH(); return NULL;
  }
};

///////////////////////////////////////////////////////////////////
/* Top level interface to breakpad.  Given a Buffer* as carefully
   acquired by the signal handler and later handed to this thread,
   unwind it.

   The first time in, read /proc/self/maps.  TODO: what about if it
   changes as we go along?

   Dump the result (PC, SP) pairs in a malloc-allocated array of
   PCandSPs, and return that and its length to the caller.  Caller is
   responsible for deallocating it.

   The first pair is for the outermost frame, the last for the
   innermost frame.
*/

MyCodeModules* sModules = NULL;
google_breakpad::LocalDebugInfoSymbolizer* sSymbolizer = NULL;

void do_breakpad_unwind_Buffer(/*OUT*/PCandSP** pairs,
                               /*OUT*/unsigned int* nPairs,
                               UnwinderThreadBuffer* buff,
                               int buffNo /* for debug printing only */)
{
# if defined(SPS_ARCH_amd64)
  MDRawContextAMD64* context = new MDRawContextAMD64();
  memset(context, 0, sizeof(*context));

  context->rip = buff->regs.rip;
  context->rbp = buff->regs.rbp;
  context->rsp = buff->regs.rsp;

  if (0) {
    LOGF("Initial RIP = 0x%llx", (unsigned long long int)context->rip);
    LOGF("Initial RSP = 0x%llx", (unsigned long long int)context->rsp);
    LOGF("Initial RBP = 0x%llx", (unsigned long long int)context->rbp);
  }

# elif defined(SPS_ARCH_arm)
  MDRawContextARM* context = new MDRawContextARM();
  memset(context, 0, sizeof(*context));

  context->iregs[7]                     = buff->regs.r7;
  context->iregs[12]                    = buff->regs.r12;
  context->iregs[MD_CONTEXT_ARM_REG_PC] = buff->regs.r15;
  context->iregs[MD_CONTEXT_ARM_REG_LR] = buff->regs.r14;
  context->iregs[MD_CONTEXT_ARM_REG_SP] = buff->regs.r13;
  context->iregs[MD_CONTEXT_ARM_REG_FP] = buff->regs.r11;

  if (0) {
    LOGF("Initial R15 = 0x%x",
         context->iregs[MD_CONTEXT_ARM_REG_PC]);
    LOGF("Initial R13 = 0x%x",
         context->iregs[MD_CONTEXT_ARM_REG_SP]);
  }

# elif defined(SPS_ARCH_x86)
  MDRawContextX86* context = new MDRawContextX86();
  memset(context, 0, sizeof(*context));

  context->eip = buff->regs.eip;
  context->ebp = buff->regs.ebp;
  context->esp = buff->regs.esp;

  if (0) {
    LOGF("Initial EIP = 0x%x", context->eip);
    LOGF("Initial ESP = 0x%x", context->esp);
    LOGF("Initial EBP = 0x%x", context->ebp);
  }

# else
#   error "Unknown plat"
# endif

  BufferMemoryRegion* memory = new BufferMemoryRegion(buff);

  if (!sModules) {
     sModules = new MyCodeModules();
  }

  if (!sSymbolizer) {
    /* Make up a list of places where the debug objects might be. */
    std::vector<std::string> debug_dirs;
#   if defined(SPS_OS_linux)
    debug_dirs.push_back("/usr/lib/debug/lib");
    debug_dirs.push_back("/usr/lib/debug/usr/lib");
    debug_dirs.push_back("/usr/lib/debug/lib/x86_64-linux-gnu");
    debug_dirs.push_back("/usr/lib/debug/usr/lib/x86_64-linux-gnu");
#   elif defined(SPS_OS_android)
    debug_dirs.push_back("/sdcard/symbols/system/lib");
    debug_dirs.push_back("/sdcard/symbols/system/bin");
#   elif defined(SPS_OS_darwin)
    /* Nothing */
#   else
#     error "Unknown plat"
#   endif
    sSymbolizer = new google_breakpad::LocalDebugInfoSymbolizer(debug_dirs);
  }

# if defined(SPS_ARCH_amd64)
  google_breakpad::StackwalkerAMD64* sw
   = new google_breakpad::StackwalkerAMD64(NULL, context,
                                           memory, sModules,
                                           sSymbolizer);
# elif defined(SPS_ARCH_arm)
  google_breakpad::StackwalkerARM* sw
   = new google_breakpad::StackwalkerARM(NULL, context,
                                         -1/*FP reg*/,
                                         memory, sModules,
                                         sSymbolizer);
# elif defined(SPS_ARCH_x86)
  google_breakpad::StackwalkerX86* sw
   = new google_breakpad::StackwalkerX86(NULL, context,
                                         memory, sModules,
                                         sSymbolizer);
# else
#   error "Unknown plat"
# endif

  google_breakpad::CallStack* stack = new google_breakpad::CallStack();

  std::vector<const google_breakpad::CodeModule*>* modules_without_symbols
    = new std::vector<const google_breakpad::CodeModule*>();
  bool b = sw->Walk(stack, modules_without_symbols);
  (void)b;
  delete modules_without_symbols;

  unsigned int n_frames = stack->frames()->size();
  unsigned int n_frames_good = 0;

  *pairs  = (PCandSP*)malloc(n_frames * sizeof(PCandSP));
  *nPairs = n_frames;
  if (*pairs == NULL) {
    *nPairs = 0;
    return;
  }

  if (n_frames > 0) {
    for (unsigned int frame_index = 0; 
         frame_index < n_frames; ++frame_index) {
      google_breakpad::StackFrame *frame = stack->frames()->at(frame_index);

      if (frame->trust == google_breakpad::StackFrame::FRAME_TRUST_CFI
          || frame->trust == google_breakpad::StackFrame::FRAME_TRUST_CONTEXT) {
        n_frames_good++;
      }

#     if defined(SPS_ARCH_amd64)
      google_breakpad::StackFrameAMD64* frame_amd64
        = reinterpret_cast<google_breakpad::StackFrameAMD64*>(frame);
      if (LOGLEVEL >= 4) {
        LOGF("frame %d   rip=0x%016llx rsp=0x%016llx    %s", 
             frame_index,
             (unsigned long long int)frame_amd64->context.rip, 
             (unsigned long long int)frame_amd64->context.rsp, 
             frame_amd64->trust_description().c_str());
      }
      (*pairs)[n_frames-1-frame_index].pc = frame_amd64->context.rip;
      (*pairs)[n_frames-1-frame_index].sp = frame_amd64->context.rsp;

#     elif defined(SPS_ARCH_arm)
      google_breakpad::StackFrameARM* frame_arm
        = reinterpret_cast<google_breakpad::StackFrameARM*>(frame);
      if (LOGLEVEL >= 4) {
        LOGF("frame %d   0x%08x   %s",
             frame_index,
             frame_arm->context.iregs[MD_CONTEXT_ARM_REG_PC],
             frame_arm->trust_description().c_str());
      }
      (*pairs)[n_frames-1-frame_index].pc
        = frame_arm->context.iregs[MD_CONTEXT_ARM_REG_PC];
      (*pairs)[n_frames-1-frame_index].sp
        = frame_arm->context.iregs[MD_CONTEXT_ARM_REG_SP];

#     elif defined(SPS_ARCH_x86)
      google_breakpad::StackFrameX86* frame_x86
        = reinterpret_cast<google_breakpad::StackFrameX86*>(frame);
      if (LOGLEVEL >= 4) {
        LOGF("frame %d   eip=0x%08x rsp=0x%08x    %s", 
             frame_index,
             frame_x86->context.eip, frame_x86->context.esp, 
             frame_x86->trust_description().c_str());
      }
      (*pairs)[n_frames-1-frame_index].pc = frame_x86->context.eip;
      (*pairs)[n_frames-1-frame_index].sp = frame_x86->context.esp;

#     else
#       error "Unknown plat"
#     endif
    }
  }

  if (LOGLEVEL >= 3) {
    LOGF("BPUnw: unwinder: seqNo %llu, buf %d: got %u frames "
         "(%u trustworthy)", 
         (unsigned long long int)buff->seqNo, buffNo, n_frames, n_frames_good);
  }

  if (LOGLEVEL >= 2) {
    if (0 == (g_stats_totalSamples % 1000))
      LOGF("BPUnw: %llu total samples, %llu failed due to buffer unavail",
           (unsigned long long int)g_stats_totalSamples,
           (unsigned long long int)g_stats_noBuffAvail);
  }

  delete stack;
  delete sw;
  delete memory;
  delete context;
}

#endif /* defined(SPS_OS_windows) */
