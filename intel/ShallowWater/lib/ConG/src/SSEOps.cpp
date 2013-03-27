#include "stdafx.h"

#include "SSEOps.h"
#include <xmmintrin.h>
#include <intrin.h>

static const unsigned int SSEMask = -16;

void SSEMulMM( const float* a, const float* b, float* r )
{
	__m128 ra, rb, rr;
	for( int i=0; i<16; i+=4 )
	{
		ra = _mm_set1_ps( a[i] );
		rb = _mm_load_ps( b );
		rr = _mm_mul_ps( ra, rb );

		for( int j=1; j<4; j++ )
		{
			ra = _mm_set1_ps( a[i+j] );
			rb = _mm_load_ps( b+(j<<2) );
			rr = _mm_add_ps( _mm_mul_ps( ra, rb ), rr );
		}
		_mm_store_ps( r+i, rr );
	}
}

void SSEMulVM( const float* a, const float* b, float* r )
{
	__m128 ra, rb, rr;
	ra = _mm_set1_ps( a[0] );
	rb = _mm_load_ps( b );
	rr = _mm_mul_ps( ra, rb );

	for ( int j=1; j<4; j++ )
	{
		ra = _mm_set1_ps( a[j] );
		rb = _mm_load_ps( b+(j<<2) );
		rr = _mm_add_ps( _mm_mul_ps( ra, rb ), rr );
	}
	_mm_storeu_ps( r, rr );
}

void SSEMulV3M( const float* a, const float a4, const float* b, float* r )
{
	__m128 ra, rb, rr;
	ra = _mm_set1_ps( a[0] );
	rb = _mm_load_ps( b );
	rr = _mm_mul_ps( ra, rb );

	ra = _mm_set1_ps( a[1] );
	rb = _mm_load_ps( b+4 );
	rr = _mm_add_ps( _mm_mul_ps( ra, rb ), rr );

	ra = _mm_set1_ps( a[2] );
	rb = _mm_load_ps( b+8 );
	rr = _mm_add_ps( _mm_mul_ps( ra, rb ), rr );

	ra = _mm_set1_ps( a4 );
	rb = _mm_load_ps( b+12 );
	rr = _mm_add_ps( _mm_mul_ps( ra, rb ), rr );

	_mm_store_ps( r, rr );
}

void SSEMulMV( const float* a, const float* b, float* r )
{
	__m128 ra, rb0, rb1, rb2, rb3, r0, r1, r2, r3, rr;
	ra = _mm_load_ps( a );
	rb0 = _mm_load_ps( b );
	rb1 = _mm_load_ps( b+4 );
	rb2 = _mm_load_ps( b+8 );
	rb3 = _mm_load_ps( b+12 );

	r0 = _mm_mul_ps( ra, rb0 );
	r1 = _mm_mul_ps( ra, rb1 );
	r2 = _mm_mul_ps( ra, rb2 );
	r3 = _mm_mul_ps( ra, rb3 );

	r0 = _mm_hadd_ps( r0, r1 );
	r2 = _mm_hadd_ps( r2, r3 );
	rr = _mm_hadd_ps( r0, r2 );

	_mm_storeu_ps( r, rr );
}

void SSEMulNM( const float a, const float* b, float* r )
{
	__m128 ra, rb, rr;
	ra = _mm_set1_ps( a );
	for( int i=0; i<16; i+=4 )
	{
		rb = _mm_load_ps( b+i );
		rr = _mm_mul_ps( ra, rb );
		_mm_store_ps( r+i, rr );
	}
}

void SSEAddMM( const float* a, const float* b, float* r )
{
	__m128 ra, rb, rr;
	for( int i=0; i<16; i+=4 )
	{
		ra = _mm_load_ps( a+i );
		rb = _mm_load_ps( b+i );
		rr = _mm_add_ps( ra, rb );
		_mm_store_ps( r+i, rr );
	}
}

void SSEMAddMM( const float* a, const float* b, const float* c, float* r )
{
	__m128 ra, rb, rc, rr;
	for( int i=0; i<16; i+=4 )
	{
		ra = _mm_set1_ps( a[i] );
		rb = _mm_load_ps( b );
		rc = _mm_load_ps( c+i );
		rr = _mm_mul_ps( ra, rb );
		
		for( int j=1; j<4; j++ )
		{
			ra = _mm_set1_ps( a[i+j] );
			rb = _mm_load_ps( b+(j<<2) );
			rr = _mm_add_ps( _mm_mul_ps( ra, rb ), rr );
		}
		rr = _mm_add_ps( rr, rc );
		_mm_store_ps( r+i, rr );
	}
}

void SSEMAddNM( const float a, const float* b, const float* c, float* r )
{
	__m128 ra, rb, rc, rr;
	ra = _mm_set1_ps( a );
	for( int i=0; i<16; i+=4 )
	{
		rb = _mm_load_ps( b+i );
		rc = _mm_load_ps( c+i );
		rr = _mm_mul_ps( ra, rb );
		rr = _mm_add_ps( rr, rc );
		_mm_store_ps( r+i, rr );
	}
}

void SSEId( float* r )
{
	memset( r, 0, SSEMatSize );
	r[0] = r[5] = r[10] = r[15] = 1;
}

float* SSEAlign( float* a )
{
	return (float*)( ( ( (unsigned int)a )+15 ) & SSEMask );
}