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

#include "../laghos_solver.hpp"
#include "qupdate.hpp"

#ifdef MFEM_USE_MPI

using namespace std;

namespace mfem {

namespace hydrodynamics {
   
// ***************************************************************************
// * qDofQuadMaps
// ***************************************************************************
static std::map<std::string, qDofQuadMaps* > AllDofQuadMaps;

// ***************************************************************************
qDofQuadMaps::~qDofQuadMaps() {}

// *****************************************************************************
void qDofQuadMaps::delqDofQuadMaps()
{
   for (std::map<std::string,
        qDofQuadMaps*>::iterator itr = AllDofQuadMaps.begin();
        itr != AllDofQuadMaps.end();
        itr++)
   {
      delete itr->second;
   }
}

// *****************************************************************************
qDofQuadMaps* qDofQuadMaps::Get(const mfem::FiniteElementSpace& fes,
                                const mfem::IntegrationRule& ir,
                                const bool transpose)
{
   return Get(*fes.GetFE(0), *fes.GetFE(0), ir, transpose);
}

qDofQuadMaps* qDofQuadMaps::Get(const mfem::FiniteElementSpace& trialFES,
                                const mfem::FiniteElementSpace& testFES,
                                const mfem::IntegrationRule& ir,
                                const bool transpose)
{
   return Get(*trialFES.GetFE(0), *testFES.GetFE(0), ir, transpose);
}

qDofQuadMaps* qDofQuadMaps::Get(const FiniteElement& trialFE,
                                const FiniteElement& testFE,
                                const IntegrationRule& ir,
                                const bool transpose)
{
   return GetTensorMaps(trialFE, testFE, ir, transpose);
}

// ***************************************************************************
qDofQuadMaps* qDofQuadMaps::GetTensorMaps(const FiniteElement& trialFE,
                                          const FiniteElement& testFE,
                                          const IntegrationRule& ir,
                                          const bool transpose)
{
   const TensorBasisElement& trialTFE =
      dynamic_cast<const TensorBasisElement&>(trialFE);
   const TensorBasisElement& testTFE =
      dynamic_cast<const TensorBasisElement&>(testFE);
   std::stringstream ss;
   ss << "TensorMap:"
      << " O1:"  << trialFE.GetOrder()
      << " O2:"  << testFE.GetOrder()
      << " BT1:" << trialTFE.GetBasisType()
      << " BT2:" << testTFE.GetBasisType()
      << " Q:"   << ir.GetNPoints();
   std::string hash = ss.str();
   // If we've already made the dof-quad maps, reuse them
   if (AllDofQuadMaps.find(hash)!=AllDofQuadMaps.end())
   {
      return AllDofQuadMaps[hash];
   }
   // Otherwise, build them
   qDofQuadMaps *maps = new qDofQuadMaps();
   AllDofQuadMaps[hash]=maps;
   maps->hash = hash;
   push();
   const qDofQuadMaps* trialMaps = GetD2QTensorMaps(trialFE, ir);
   const qDofQuadMaps* testMaps  = GetD2QTensorMaps(testFE, ir, true);
   maps->dofToQuad   = trialMaps->dofToQuad;
   maps->dofToQuadD  = trialMaps->dofToQuadD;
   maps->quadToDof   = testMaps->dofToQuad;
   maps->quadToDofD  = testMaps->dofToQuadD;
   maps->quadWeights = testMaps->quadWeights;
   pop();
   return maps;
}

// ***************************************************************************
qDofQuadMaps* qDofQuadMaps::GetD2QTensorMaps(const FiniteElement& fe,
                                             const IntegrationRule& ir,
                                             const bool transpose)
{
   const mfem::TensorBasisElement& tfe = dynamic_cast<const TensorBasisElement&>
                                         (fe);
   const Poly_1D::Basis& basis = tfe.GetBasis1D();
   const int order = fe.GetOrder();
   const int dofs = order + 1;
   const int dims = fe.GetDim();
   const mfem::IntegrationRule& ir1D = IntRules.Get(Geometry::SEGMENT,
                                                    ir.GetOrder());
   const int quadPoints = ir1D.GetNPoints();
   const int quadPoints2D = quadPoints*quadPoints;
   const int quadPoints3D = quadPoints2D*quadPoints;
   const int quadPointsND = ((dims == 1) ? quadPoints :
                             ((dims == 2) ? quadPoints2D : quadPoints3D));
   std::stringstream ss ;
   ss << "D2QTensorMap:"
      << " order:" << order
      << " dofs:" << dofs
      << " dims:" << dims
      << " quadPoints:"<<quadPoints
      << " transpose:"  << (transpose?"T":"F");
   std::string hash = ss.str();
   if (AllDofQuadMaps.find(hash)!=AllDofQuadMaps.end())
   {
      return AllDofQuadMaps[hash];
   }

   push();
   qDofQuadMaps *maps = new qDofQuadMaps();
   AllDofQuadMaps[hash]=maps;
   maps->hash = hash;

   maps->dofToQuad.allocate(quadPoints, dofs,1,1,transpose);
   maps->dofToQuadD.allocate(quadPoints, dofs,1,1,transpose);
   double* quadWeights1DData = NULL;
   if (transpose)
   {
      // Initialize quad weights only for transpose
      maps->quadWeights.allocate(quadPointsND);
      quadWeights1DData = ::new double[quadPoints];
   }
   mfem::Vector d2q(dofs);
   mfem::Vector d2qD(dofs);
   mfem::Array<double> dofToQuad(quadPoints*dofs);
   mfem::Array<double> dofToQuadD(quadPoints*dofs);
   for (int q = 0; q < quadPoints; ++q)
   {
      const IntegrationPoint& ip = ir1D.IntPoint(q);
      basis.Eval(ip.x, d2q, d2qD);
      if (transpose)
      {
         quadWeights1DData[q] = ip.weight;
      }
      for (int d = 0; d < dofs; ++d)
      {
         dofToQuad[maps->dofToQuad.dim()[0]*q + maps->dofToQuad.dim()[1]*d] = d2q[d];
         dofToQuadD[maps->dofToQuad.dim()[0]*q + maps->dofToQuad.dim()[1]*d] = d2qD[d];
      }
   }
   maps->dofToQuad = dofToQuad;
   maps->dofToQuadD = dofToQuadD;
   if (transpose)
   {
      mfem::Array<double> quadWeights(quadPointsND);
      for (int q = 0; q < quadPointsND; ++q)
      {
         const int qx = q % quadPoints;
         const int qz = q / quadPoints2D;
         const int qy = (q - qz*quadPoints2D) / quadPoints;
         double w = quadWeights1DData[qx];
         if (dims > 1)
         {
            w *= quadWeights1DData[qy];
         }
         if (dims > 2)
         {
            w *= quadWeights1DData[qz];
         }
         quadWeights[q] = w;
      }
      maps->quadWeights = quadWeights;
      ::delete [] quadWeights1DData;
   }
   assert(maps);
   pop();
   return maps;
}

// ***************************************************************************
qDofQuadMaps* qDofQuadMaps::GetSimplexMaps(const FiniteElement& fe,
                                           const IntegrationRule& ir,
                                           const bool transpose)
{
   return GetSimplexMaps(fe, fe, ir, transpose);
}

// *****************************************************************************
qDofQuadMaps* qDofQuadMaps::GetSimplexMaps(const FiniteElement& trialFE,
                                           const FiniteElement& testFE,
                                           const IntegrationRule& ir,
                                           const bool transpose)
{
   std::stringstream ss;
   ss << "SimplexMap:"
      << " O1:" << trialFE.GetOrder()
      << " O2:" << testFE.GetOrder()
      << " Q:"  << ir.GetNPoints();
   std::string hash = ss.str();
   // If we've already made the dof-quad maps, reuse them
   if (AllDofQuadMaps.find(hash)!=AllDofQuadMaps.end())
   {
      return AllDofQuadMaps[hash];
   }
   push();
   qDofQuadMaps *maps = new qDofQuadMaps();
   AllDofQuadMaps[hash]=maps;
   maps->hash = hash;
   const qDofQuadMaps* trialMaps = GetD2QSimplexMaps(trialFE, ir);
   const qDofQuadMaps* testMaps  = GetD2QSimplexMaps(testFE, ir, true);
   maps->dofToQuad   = trialMaps->dofToQuad;
   maps->dofToQuadD  = trialMaps->dofToQuadD;
   maps->quadToDof   = testMaps->dofToQuad;
   maps->quadToDofD  = testMaps->dofToQuadD;
   maps->quadWeights = testMaps->quadWeights;
   pop();
   return maps;
}

// ***************************************************************************
qDofQuadMaps* qDofQuadMaps::GetD2QSimplexMaps(const FiniteElement& fe,
                                              const IntegrationRule& ir,
                                              const bool transpose)
{
   const int dims = fe.GetDim();
   const int numDofs = fe.GetDof();
   const int numQuad = ir.GetNPoints();
   std::stringstream ss ;
   ss << "D2QSimplexMap:"
      << " Dim:" << dims
      << " numDofs:" << numDofs
      << " numQuad:" << numQuad
      << " transpose:"  << (transpose?"T":"F");
   std::string hash = ss.str();
   if (AllDofQuadMaps.find(hash)!=AllDofQuadMaps.end())
   {
      return AllDofQuadMaps[hash];
   }
   qDofQuadMaps* maps = new qDofQuadMaps();
   AllDofQuadMaps[hash]=maps;
   maps->hash = hash;
   push(SteelBlue);
   dbg("Initialize the dof -> quad mapping");
   maps->dofToQuad.allocate(numQuad, numDofs,1,1,transpose);
   maps->dofToQuadD.allocate(dims, numQuad, numDofs,1,transpose);
   if (transpose) // Initialize quad weights only for transpose
   {
      maps->quadWeights.allocate(numQuad);
   }
   dbg("d2q");
   mfem::Vector d2q(numDofs);
   mfem::DenseMatrix d2qD(numDofs, dims);
   mfem::Array<double> quadWeights(numQuad);
   mfem::Array<double> dofToQuad(numQuad*numDofs);
   mfem::Array<double> dofToQuadD(dims*numQuad*numDofs);
   for (int q = 0; q < numQuad; ++q)
   {
      const IntegrationPoint& ip = ir.IntPoint(q);
      if (transpose)
      {
         quadWeights[q] = ip.weight;
      }
      fe.CalcShape(ip, d2q);
      fe.CalcDShape(ip, d2qD);
      for (int d = 0; d < numDofs; ++d)
      {
         const double w = d2q[d];
         dofToQuad[maps->dofToQuad.dim()[0]*q +
                                              maps->dofToQuad.dim()[1]*d] = w;
         for (int dim = 0; dim < dims; ++dim)
         {
            const double wD = d2qD(d, dim);
            dofToQuadD[maps->dofToQuadD.dim()[0]*dim +
                       maps->dofToQuadD.dim()[1]*q +
                       maps->dofToQuadD.dim()[2]*d] = wD;
         }
      }
   }
   if (transpose)
   {
      maps->quadWeights = quadWeights;
   }
   maps->dofToQuad = dofToQuad;
   maps->dofToQuadD = dofToQuadD;
   pop();
   dbg("done");
   return maps;
}

} // namespace hydrodynamics
   
} // namespace mfem

#endif // defined(MFEM_USE_BACKENDS) && defined(MFEM_USE_KERNELS)
