// Copyright (c) 2017, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-734707. All Rights
// reserved. See files LICENSE and NOTICE for details.
//
// This file is part of CEED, a collection of benchmarks, miniapps, software
// libraries and APIs for efficient high-order finite element and spectral
// element discretizations for exascale applications. For more information and
// source code availability see http://github.com/ceed.
//
// The CEED research is supported by the Exascale Computing Project 17-SC-20-SC,
// a collaborative effort of two U.S. Department of Energy organizations (Office
// of Science and the National Nuclear Security Administration) responsible for
// the planning and preparation of a capable exascale ecosystem, including
// software, applications, hardware, advanced system engineering and early
// testbed platforms, in support of the nation's exascale computing imperative.
#include "../kernels.hpp"

// *****************************************************************************
void rForceMult2D(const int, const int, const int, const int, const int,
                  const int, const double*, const double*, const double*,
                  const double*, const double*, double*);

void rForceMultTranspose2D(const int, const int, const int, const int,
                           const int,  const int, const double*, const double*,
                           const double*, const double*, const double*, double*);

void rForceMult3D(const int, const int, const int, const int, const int,
                  const int, const double*, const double*, const double*,
                  const double*, const double*, double* );

void rForceMultTranspose3D(const int, const int, const int, const int,
                           const int,
                           const int, const double*, const double*, const double*,
                           const double*, const double*, double*);

// *****************************************************************************
void rForceMult(const int NUM_DIM,
                const int NUM_DOFS_1D,
                const int NUM_QUAD_1D,
                const int L2_DOFS_1D,
                const int H1_DOFS_1D,
                const int nzones,
                const double* L2QuadToDof,
                const double* H1DofToQuad,
                const double* H1DofToQuadD,
                const double* stressJinvT,
                const double* e,
                double* v) {
   if (NUM_DIM==2)
      rForceMult2D(NUM_DIM,
                   NUM_DOFS_1D,
                   NUM_QUAD_1D,
                   L2_DOFS_1D,
                   H1_DOFS_1D,
                   nzones,
                   L2QuadToDof,
                   H1DofToQuad,
                   H1DofToQuadD,
                   stressJinvT,
                   e,
                   v);

   if (NUM_DIM==3)
      rForceMult3D(NUM_DIM,
                   NUM_DOFS_1D,
                   NUM_QUAD_1D,
                   L2_DOFS_1D,
                   H1_DOFS_1D,
                   nzones,
                   L2QuadToDof,
                   H1DofToQuad,
                   H1DofToQuadD,
                   stressJinvT,
                   e,
                   v);

   if (NUM_DIM!=2 && NUM_DIM!=3) {
      exit(printf("\n[rForceMult] NUM_DIM!=2 && NUM_DIM!=3 ERROR"));
   }
}

// *****************************************************************************
void rForceMultTranspose(const int NUM_DIM,
                         const int NUM_DOFS_1D,
                         const int NUM_QUAD_1D,
                         const int L2_DOFS_1D,
                         const int H1_DOFS_1D,
                         const int nzones,
                         const double* L2QuadToDof,
                         const double* H1DofToQuad,
                         const double* H1DofToQuadD,
                         const double* stressJinvT,
                         const double* v,
                         double* e) {
   if (NUM_DIM==2)
      rForceMultTranspose2D(NUM_DIM,
                            NUM_DOFS_1D,
                            NUM_QUAD_1D,
                            L2_DOFS_1D,
                            H1_DOFS_1D,
                            nzones,
                            L2QuadToDof,
                            H1DofToQuad,
                            H1DofToQuadD,
                            stressJinvT,
                            v,
                            e);

   if (NUM_DIM==3)
      rForceMultTranspose3D(NUM_DIM,
                            NUM_DOFS_1D,
                            NUM_QUAD_1D,
                            L2_DOFS_1D,
                            H1_DOFS_1D,
                            nzones,
                            L2QuadToDof,
                            H1DofToQuad,
                            H1DofToQuadD,
                            stressJinvT,
                            v,
                            e);

   if (NUM_DIM!=2 && NUM_DIM!=3) {
      exit(printf("\n[rForceMultTranspose] NUM_DIM!=2 && NUM_DIM!=3 ERROR"));
   }
}

