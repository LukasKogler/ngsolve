/**********************************************************************/
/* File:   localsolve.cpp                                             */
/* Author: Joachim Schoeberl                                          */
/* Date:   12. May. 2020                                              */
/**********************************************************************/

/* 
   local solve 
   useful for equilibration
*/

#include <comp.hpp>
#include <variant>
#include "../fem/integratorcf.hpp"


namespace ngcomp
{
  class HatFunction : public T_CoefficientFunction<HatFunction>
  {
    size_t vnum;
  public:
    void SetVertexNumber (size_t avnum) { vnum = avnum; }

    template <typename MIR, typename T, ORDERING ORD>
    void T_Evaluate (const MIR & ir, BareSliceMatrix<T,ORD> values) const
    {
      auto & trafo = ir.GetTransformation();
      auto ma = (MeshAccess*)trafo.GetMesh();
      auto ei = trafo.GetElementId();
      auto vnums = ma->GetElVertices(ei);
      auto locvi = vnums.Pos(vnum);

      ScalarFE<ET_SEGM,1> fel_segm;
      ScalarFE<ET_TRIG,1> fel_trig;
      ScalarFE<ET_QUAD,1> fel_quad;
      BaseScalarFiniteElement * fel;
      
      switch (trafo.GetElementType())
        {
        case ET_SEGM: fel =  &fel_segm; break;          
        case ET_TRIG: fel =  &fel_trig; break;
        case ET_QUAD: fel =  &fel_quad; break;
        default:
          throw Exception("HatFunction - unhandled element-type "+ToString(trafo.GetElementType()));
        }
      if constexpr (is_same<T,SIMD<double>>::value ||
                    is_same<T,double>::value)
                     {
                       STACK_ARRAY(T, mem, fel->GetNDof()*ir.Size());
                       FlatMatrix<T> shapes(fel->GetNDof(), ir.Size(), &mem[0]);
                       fel->CalcShape(ir.IR(), shapes);
                       values.Row(0).Range(0,ir.Size()) = shapes.Row(locvi);
                     }
      else
        cout << "can evaluate only for double or simd<double>" << endl;
    }
    
    template <typename MIR, typename T, ORDERING ORD>
    void T_Evaluate (const MIR & ir,
                     FlatArray<BareSliceMatrix<T,ORD>> input,                       
                     BareSliceMatrix<T,ORD> values) const
    {
      T_Evaluate (ir, values);
    }    
  };

  
  void PatchwiseSolve (shared_ptr<SumOfIntegrals> bf,
                       shared_ptr<SumOfIntegrals> lf,
                       shared_ptr<GridFunction> gf)
  {
    LocalHeap lh(10*1000*1000, "LocalSolve LocalHeap");
    auto ma = gf->GetMeshAccess();
    auto fes = gf->GetFESpace();
    const BitArray & freedofs = *fes->GetFreeDofs();

    Array<shared_ptr<BilinearFormIntegrator>> bfis[4];  // VOL, BND, ...
    Array<shared_ptr<LinearFormIntegrator>> lfis[4];
    auto gfvec = gf->GetVector().FV<double>();
    gfvec = 0.0;
    
    for (auto icf : bf->icfs)
      {
        auto & dx = icf->dx;
        bfis[dx.vb] += make_shared<SymbolicBilinearFormIntegrator> (icf->cf, dx.vb, dx.element_vb);
      }        

    auto hatfunc = make_shared<HatFunction> ();
    for (auto icf : lf->icfs)
      {
        auto & dx = icf->dx;
        lfis[dx.vb] += make_shared<SymbolicLinearFormIntegrator> (hatfunc*icf->cf, dx.vb, dx.element_vb);
      }        

    
    
    
    Array<DofId> patchdofs, dofs, el2patch;
    
    for (auto v : ma->Vertices())
      {
        // cout << "vertex " << size_t(v) << endl;
        HeapReset hr(lh);
        hatfunc -> SetVertexNumber(v);
        
        patchdofs.SetSize0();
        for (auto vb : { VOL, BND })
          for (auto el : ma->GetVertexElements(v, vb))
            {
              fes->GetDofNrs(ElementId(vb, el), dofs);
              for (auto d : dofs)
                if (freedofs.Test(d))
                  if (!patchdofs.Contains(d))
                    patchdofs.Append(d);
            }

        FlatMatrix<> patchmat(patchdofs.Size(), lh);
        FlatVector<> patchvec(patchdofs.Size(), lh);        
        FlatVector<> patchsol(patchdofs.Size(), lh);        
        patchmat = 0.0;
        patchvec = 0.0;

        for (auto vb : { VOL, BND })
          for (auto el : ma->GetVertexElements(v, vb))
            {
              ElementId ei(vb, el);
              // cout << "ei = " << ei << endl;
              fes->GetDofNrs(ei, dofs);
              // cout << "dofs = " << dofs << endl;
              auto & trafo = ma->GetTrafo(ei, lh);
              auto & fel = fes->GetFE(ei, lh);
              
              
              el2patch.SetSize(dofs.Size());
              for (auto i : Range(dofs))
                el2patch[i] = patchdofs.Pos(dofs[i]);
              // cout << "el2patch = " << el2patch << endl;
              
              FlatMatrix<> elmat(dofs.Size(), lh);
              elmat = 0.0;
              for (auto & bfi : bfis[vb])
                bfi -> CalcElementMatrixAdd(fel, trafo, elmat, lh);
              // cout << "el2patch = " << el2patch << endl;
              // cout << "elmat = " << elmat << endl;
              
              FlatVector<> elvec(dofs.Size(), lh);
              FlatVector<> sumelvec(dofs.Size(), lh);
              sumelvec = 0.0;
              for (auto & lfi : lfis[vb])
                {
                  lfi -> CalcElementVector(fel, trafo, elvec, lh);
                  sumelvec += elvec;
                }
              
              // cout << "el2patch = " << el2patch << endl;
              for (auto i : Range(dofs))
                for (auto j : Range(dofs))
                  if (el2patch[i] != Array<DofId>::ILLEGAL_POSITION &&
                      el2patch[j] != Array<DofId>::ILLEGAL_POSITION)
                    patchmat(el2patch[i], el2patch[j]) += elmat(i,j);
              for (auto i : Range(dofs))
                if (el2patch[i] != Array<DofId>::ILLEGAL_POSITION)
                  patchvec(el2patch[i]) += sumelvec(i);
            }
        
        
        // cout << "patchmat = " << endl << patchmat << endl;
        Matrix<> inv_patchmat = patchmat;
        CalcInverse (inv_patchmat);
        // cout << "inv patchmat = " << endl << patchmat << endl;        
        patchsol = inv_patchmat * patchvec;
        // Vector<> res = patchvec - patchmat * patchsol;  // need post-iteration for high penalty
        // patchsol += inv_patchmat * res;
        // cout << "patchvec = " << endl << patchvec << endl;
        // cout << "patchsol = " << endl << patchsol << endl;
        for (auto i : Range(patchdofs))
          gfvec(patchdofs[i]) += patchsol(i);
      }
  }
}
  
