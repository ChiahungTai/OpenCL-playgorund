#pragma once

//aligned and unaligned matrix types
typedef	__declspec( align( 64 ) ) float	SSEMat[16];
typedef float SSEUnaligned[20];

//matrix size
const int SSEMatSize = 16*sizeof( float );

//M = matrix, V = vector, N = number
void SSEMulMM( const float* a, const float* b, float* r);
void SSEMulVM( const float* a, const float* b, float* r);
void SSEMulV3M( const float* a, const float a4, const float* b, float* r);
void SSEMulMV( const float* a, const float* b, float* r);
void SSEMulNM( const float a, const float* b, float* r);
void SSEAddMM( const float* a, const float* b, float* r);
void SSEMAddMM( const float* a, const float* b, const float* c, float* r);
void SSEMAddNM( const float a, const float* b, const float* c, float* r);
//identity matrix
void SSEId( float* r );
//align pointer
float* SSEAlign( float* a );

