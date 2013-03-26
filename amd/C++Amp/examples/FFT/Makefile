DEPTH = ../../../..

include $(DEPTH)/make/openclsdkdefs.mk 

####
#
#  Targets
#
####

CPPAMP			= 1
ifeq ($(strip $(CL_LIB_VER)),vc110)
SAMPLE_EXE		= 1
EXE_TARGET 		= FFT
EXE_TARGET_INSTALL   	= FFT

C_RELEASE_FLAG := $(subst /O2,/O1,$(C_RELEASE_FLAG))
C_DEBUG_FLAG := $(subst /RTC1,/O1,$(C_DEBUG_FLAG))

####
#
#  C/CPP files
#
####

FILES 		= FFT

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

