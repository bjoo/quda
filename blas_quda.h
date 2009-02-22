#include <cuComplex.h>
#include <enum_quda.h>

#ifndef _QUDA_BLAS_H
#define _QUDA_BLAS_H

#if QudaSumFloat==double
#define QudaSumComplex cuDoubleComplex
#define QudaSumFloat3 double3
#else
#define QudaSumComplex cuComplex
#define QudaSumFloat3 float3
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------- blas_cuda.cu ----------

void zeroCuda(float* dst, int cnt);
void copyCuda(float* dst, float *src, int len);

void axpbyCuda(float a, float *x, float b, float *y, int len);
void axpyCuda(float a, float *x, float *y, int len);
void axCuda(float a, float *x, int len);
void xpayCuda(float *x, float a, float *y, int len);
void mxpyCuda(float *x, float *y, int len);

void axpyZpbxCuda(float a, float *x, float *y, float *z, float b, int len);
QudaSumFloat axpyNormCuda(float a, float *x, float *y, int len);

QudaSumFloat sumCuda(float *a, int n);
QudaSumFloat normCuda(float *a, int n);
QudaSumFloat reDotProductCuda(float *a, float *b, int n);

void blasTest();
void axpbyTest();

void caxpbyCuda(float2 a, float2 *x, float2 b, float2 *y, int len);
void caxpyCuda(float2 a, float2 *x, float2 *y, int len);
void cxpaypbzCuda(float2 *x, float2 b, float2 *y, float2 c, float2 *z, int len);
QudaSumComplex cDotProductCuda(float2*, float2*, int len);
void caxpbypzYmbwCuda(float2, float2*, float2, float2*, float2*, float2*, int len);
QudaSumFloat3 cDotProductNormACuda(float2 *a, float2 *b, int n);
QudaSumFloat3 cDotProductNormBCuda(float2 *a, float2 *b, int n);
QudaSumFloat3 caxpbypzYmbwcDotProductWYNormYCuda(float2 a, float2 *x, float2 b, float2 *y, 
						 float2 *z, float2 *w, float2 *u, int len);

QudaSumComplex xpaycDotzyCuda(float2 *x, float a, float2 *y, float2 *z, int len);

// ---------- blas_reference.cpp ----------

void zero(float* a, int cnt);
void copy(float* a, float *b, int len);

void ax(float a, float *x, int len);

void axpbyCuda(float a, float *x, float b, float *y, int len);
void axpy(float a, float *x, float *y, int len);
void xpay(float *x, float a, float *y, int len);
void mxpy(float *x, float *y, int len);

float norm(float *vector, int len);
float reDotProduct(float *v1, float *v2, int len);
float imDotProduct(float *v1, float *v2, int len);
double normD(float *vector, int len);
double reDotProductD(float *v1, float *v2, int len);
double imDotProductD(float *v1, float *v2, int len);


#ifdef __cplusplus
}
#endif

#endif // _QUDA_BLAS_H
