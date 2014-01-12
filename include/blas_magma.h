#ifndef _BLAS_MAGMA_H
#define _BLAS_MAGMA_H

#include <cuda.h>
#include <cuda_runtime.h>
#include <string>
#include <complex>
#include <cuComplex.h>
#include <stdio.h>

#ifndef MAX
#define MAX(a, b) (a > b) ? a : b;
#endif

//magma library interface 
//required for (incremental) EigCG solver

   class BlasMagmaArgs{
    private:

      //problem sizes:
      int m;
      int nev;
      int prec;
      int ldm;//(may include padding)

      //general magma library parameters:	
      int info;

      bool init;
      bool alloc;

      //magma params/objects:
      int llwork; 
      int lrwork;
      int liwork;

      int sideLR;

      int htsize;//MIN(l,k)-number of Householder reflectors, but we always have k <= MIN(m,n)
      int dtsize;//in general: MIN(m,k) for side = 'L' and MIN(n,k) for side = 'R'

      int lwork_max; 

      void *W;
      void *W2;
      void *hTau;
      void *dTau;

      void *lwork;
      void *rwork;
      int  *iwork;

    public:
      BlasMagmaArgs(const int prec);
      BlasMagmaArgs(const int m, const int nev, const int ldm, const int prec);

      ~BlasMagmaArgs();

      //Collection of methods for EigCG solver:
      void MagmaHEEVD(void *dTvecm, void *hTvalm, const int problem_size);
      //
      int  MagmaORTH_2nev(void *dTvecm, void *dTm);
      //
      void RestartV(void *dV, int ldn, void *dTevecm, void *dTm);

      //Collection of methods used for the initial guess vector deflation:

      //this accepts host routines, and employ either CPU or GPU, depending on problem size etc.
      void SolveProjMatrix(void* rhs, const int ldn, const int n, void* H, const int ldH);

      //GPU version of the above
      void SolveGPUProjMatrix(void* rhs, const int ldn, const int n, void* H, const int ldH);
      
      //Spinor matrix vector product:
      void SpinorMatVec(void *spinorOut, const void *spinorSetIn, const void *vec, const int slen, const int vlen);
   };


#endif // _BLAS_MAGMA_H
