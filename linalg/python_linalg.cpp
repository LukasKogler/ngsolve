#ifdef NGS_PYTHON
#include <la.hpp>
#include "../ngstd/python_ngstd.hpp"
using namespace ngla;

MSVC2015_UPDATE3_GET_PTR_FIX(ngla::BaseMatrix)
MSVC2015_UPDATE3_GET_PTR_FIX(ngla::BaseVector)
MSVC2015_UPDATE3_GET_PTR_FIX(ngla::CGSolver<class std::complex<double> >)
MSVC2015_UPDATE3_GET_PTR_FIX(ngla::CGSolver<double>)
MSVC2015_UPDATE3_GET_PTR_FIX(ngla::QMRSolver<double>)
MSVC2015_UPDATE3_GET_PTR_FIX(ngla::QMRSolver<Complex>)
MSVC2015_UPDATE3_GET_PTR_FIX(ngla::GMRESSolver<double>)
MSVC2015_UPDATE3_GET_PTR_FIX(ngla::GMRESSolver<Complex>)
MSVC2015_UPDATE3_GET_PTR_FIX(ngla::Projector)

static void InitSlice( const bp::slice &inds, int len, int &start, int &step, int &n ) {
    bp::object indices = inds.attr("indices")(len);
    start = 0;
    step = 1;
    n = 0;

    try {
        start = bp::extract<int>(indices[0]);
        int stop  = bp::extract<int>(indices[1]);
        step  = bp::extract<int>(indices[2]);
        n = (stop-start+step-1) / step;
        if(step>1)
            bp::exec("raise IndexError('slices with step>1 not supported')\n");
    }
    catch (bp::error_already_set const &) {
        cout << "Error in InitSlice(slice,...): " << endl;
        PyErr_Print();
    }

  if (is_complex)
    res = make_shared<VVector<Complex>> (size);
  else
    res = make_shared<VVector<double>> (size);
  return res;
}

void NGS_DLL_HEADER ExportNgla(py::module &m) {
    cout << IM(1) << "exporting linalg" << endl;
    // TODO
//     py::object expr_module = py::import("ngsolve.__expr");
//     py::object expr_namespace = expr_module.attr("__dict__");


//     struct BaseVector_pickle_suite : py::pickle_suite
//     {
//       static
//       py::tuple getinitargs(const BaseVector & v)
//       {
//         return py::make_tuple(v.Size(), v.IsComplex(), v.EntrySize()); 
//       }
// 
//       static
//       py::tuple getstate(py::object obj)
//       {
//         /*
//         const BaseVector & vec = py::extract<BaseVector const&>(obj)();
//         py::list data;
//         for (int i : Range(vec))
//           data.append (vec.FV<double>()(i));
//         */
//         auto vec = py::extract<BaseVector const&>(obj)().FV<double>();
//         py::list data;
//         for (int i : Range(vec))
//           data.append (vec(i));
//         return py::make_tuple (obj.attr("__dict__"), data);
//       }
//     
//       static
//       void setstate(py::object obj, py::tuple state)
//       {
//         py::dict d = py::extract<py::dict>(obj.attr("__dict__"))();
//         d.update(state[0]);
//         py::list data = py::extract<py::list>(state[1]);
//         auto vec = py::extract<BaseVector const&>(obj)().FV<double>();
//         for (int i : Range(vec))
//           vec(i) = py::extract<double> (data[i]);
//         /*
//         BaseVector & vec = py::extract<BaseVector&>(obj)();
//         for (int i : Range(vec))
//           vec.FV<double>()(i) = py::extract<double> (data[i]);
//         */
//       }
// 
//     static bool getstate_manages_dict() { return true; }
//   };
    
    
  
  REGISTER_PTR_TO_PYTHON_BOOST_1_60_FIX(shared_ptr<BaseVector>);
  bp::class_<BaseVector, shared_ptr<BaseVector>, boost::noncopyable>("BaseVector", bp::no_init)
    .def("__init__", bp::make_constructor 
         (FunctionPointer ([](int size, bool is_complex, int es) -> shared_ptr<BaseVector>
                           {
			     if(es > 1)
			       {
				 if(is_complex)
				   return make_shared<S_BaseVectorPtr<Complex>> (size, es);
				 else
				   return make_shared<S_BaseVectorPtr<double>> (size, es);
			       }
			     
			     if (is_complex)
                               return make_shared<VVector<Complex>> (size);
                             else
                               return make_shared<VVector<double>> (size);
                           }),
          bp::default_call_policies(),        // need it to use argumentso
          (bp::arg("size"), bp::arg("complex")=false, bp::arg("entrysize")=1)
          ))
    .def_pickle(BaseVector_pickle_suite())
    .def("__ngsid__", FunctionPointer( [] ( BaseVector & self)
        { return reinterpret_cast<std::uintptr_t>(&self); } ) )
    
    .def("__str__", &ToString<BaseVector>)
    .add_property("size", &BaseVector::Size)
    .def("__len__", &BaseVector::Size)
    .def("CreateVector", FunctionPointer( [] ( BaseVector & self)
        { return shared_ptr<BaseVector>(self.CreateVector()); } ))

    /*
    .def("Assign", FunctionPointer([](BaseVector & self, BaseVector & v2, double s)->void { self.Set(s, v2); }))
    .def("Add", FunctionPointer([](BaseVector & self, BaseVector & v2, double s)->void { self.Add(s, v2); }))
    .def("Assign", FunctionPointer([](BaseVector & self, BaseVector & v2, Complex s)->void { self.Set(s, v2); }))
    .def("Add", FunctionPointer([](BaseVector & self, BaseVector & v2, Complex s)->void { self.Add(s, v2); }))
    */
    .def("Assign", FunctionPointer([](BaseVector & self, BaseVector & v2, bp::object s)->void 
                                   { 
                                     if ( bp::extract<double>(s).check() )
                                       {
                                         self.Set (bp::extract<double>(s)(), v2);
                                         return;
                                       }
                                     if ( bp::extract<Complex>(s).check() )
                                       {
                                         self.Set (bp::extract<Complex>(s)(), v2);
                                         return;
                                       }
                                     throw Exception ("BaseVector::Assign called with non-scalar type");
                                   }))
    .def("Add", FunctionPointer([](BaseVector & self, BaseVector & v2, bp::object s)->void 
                                   { 
                                     if ( bp::extract<double>(s).check() )
                                       {
                                         self.Add (bp::extract<double>(s)(), v2);
                                         return;
                                       }
                                     if ( bp::extract<Complex>(s).check() )
                                       {
                                         self.Add (bp::extract<Complex>(s)(), v2);
                                         return;
                                       }
                                     throw Exception ("BaseVector::Assign called with non-scalar type");
                                   }))


    .add_property("expr", bp::object(expr_namespace["VecExpr"]) )
    .add_property("data", bp::object(expr_namespace["VecExpr"]), bp::object(expr_namespace["expr_data"] ))
    .def("__add__" , bp::object(expr_namespace["expr_add"]) )
    .def("__sub__" , bp::object(expr_namespace["expr_sub"]) )
    .def("__rmul__" , bp::object(expr_namespace["expr_rmul"]) )
    .def("__getitem__", FunctionPointer
         ( [](BaseVector & self,  int ind )
           {
             if (ind < 0 || ind >= self.Size()) 
               bp::exec("raise IndexError()\n");
             int entrysize = self.EntrySize();
             if( self.IsComplex() ) entrysize/=2;
             if(entrysize == 1)
             {
                 if( !self.IsComplex() )
                     return bp::object(self.FVDouble()[ind]);
                 else
                     return bp::object(self.FVComplex()[ind]);
             }
             else
             {
                 // return FlatVector<T>
                 if( self.IsComplex() )
                   return bp::object(self.SV<Complex>()(ind));
                 else
                   return bp::object(self.SV<double>()(ind));
             }
           } ))
    .def("__getitem__", FunctionPointer( [](BaseVector & self,  bp::slice inds )
      {
          int start, step, n;
          InitSlice( inds, self.Size(), start, step, n );
          return shared_ptr<BaseVector>(self.Range(start, start+n));
      } ))
    .def("__setitem__", FunctionPointer( [](BaseVector & self,  int ind, Complex z )
      {
          self.Range(ind,ind+1) = z;
      } ))
    .def("__setitem__", FunctionPointer( [](BaseVector & self,  int ind, double d )
      {
          self.Range(ind,ind+1) = d;
      } ))
    .def("__setitem__", FunctionPointer( [](BaseVector & self,  bp::slice inds, Complex z )
      {
          int start, step, n;
          InitSlice( inds, self.Size(), start, step, n );
          self.Range(start,start+n) = z;
      } ))
    .def("__setitem__", FunctionPointer( [](BaseVector & self,  bp::slice inds, double d )
      {
          int start, step, n;
          InitSlice( inds, self.Size(), start, step, n );
          self.Range(start,start+n) = d;
      } ))
    .def("__setitem__", FunctionPointer( [](BaseVector & self,  int ind, FlatVector<Complex> & v )
      {
          if( self.IsComplex() )
            self.SV<Complex>()(ind) = v;
          else
            bp::exec("raise IndexError('cannot assign complex values to real vector')\n");
      } ))
    .def("__setitem__", FunctionPointer( [](BaseVector & self,  int ind, FlatVector<double> & v )
      {
          if( self.IsComplex() )
            self.SV<Complex>()(ind) = v;
          else
            self.SV<double>()(ind) = v;
      } ))
    .def(bp::self+=bp::self)
    .def(bp::self-=bp::self)
    .def(bp::self*=double())
    .def("InnerProduct", FunctionPointer( [](BaseVector & self, BaseVector & other)
                                          {
                                            if (self.IsComplex())
                                              return bp::object (S_InnerProduct<ComplexConjugate> (self, other));
                                            else
                                              return bp::object (InnerProduct (self, other));
                                          }))
    .def("Norm", FunctionPointer ( [](BaseVector & self) { return self.L2Norm(); }))
    .def("Range", FunctionPointer( [](BaseVector & self, int from, int to) -> shared_ptr<BaseVector>
                                   {
                                     return self.Range(from,to);
                                   }))
    .def("FV", FunctionPointer( [] (BaseVector & self) -> FlatVector<double>
                                {
                                  return self.FVDouble();
                                }))
    ;       

  // bp::def("InnerProduct", FunctionPointer([](BaseVector & v1, BaseVector & v2)->double { return InnerProduct(v1,v2); }))
  bp::def ("InnerProduct",
           FunctionPointer( [] (bp::object x, bp::object y) -> bp::object
                            { return x.attr("InnerProduct") (y); }));
  ;
  

  typedef BaseMatrix BM;
  typedef BaseVector BV;

  REGISTER_PTR_TO_PYTHON_BOOST_1_60_FIX(shared_ptr<BaseMatrix>);
  bp::class_<BaseMatrix, shared_ptr<BaseMatrix>, boost::noncopyable>("BaseMatrix", bp::no_init)
    .def("__str__", &ToString<BaseMatrix>)
    .add_property("height", &BaseMatrix::Height)
    .add_property("width", &BaseMatrix::Width)

    .def("CreateMatrix", &BaseMatrix::CreateMatrix)

    .def("CreateRowVector", FunctionPointer( [] ( BaseMatrix & self)
        { return shared_ptr<BaseVector>(self.CreateRowVector()); } ))
    .def("CreateColVector", FunctionPointer( [] ( BaseMatrix & self)
        { return shared_ptr<BaseVector>(self.CreateColVector()); } ))

    .def("AsVector", FunctionPointer( [] (BM & m)
                                      {
                                        return shared_ptr<BaseVector> (&m.AsVector(), NOOP_Deleter);
                                      }))
    .def("COO", FunctionPointer( [] (BM & m) -> bp::object
                                 {
                                   SparseMatrix<double> * sp = dynamic_cast<SparseMatrix<double>*> (&m);
                                   if (sp)
                                     {
                                       Array<int> ri, ci;
                                       Array<double> vals;
                                       for (int i = 0; i < sp->Height(); i++)
                                         {
                                           FlatArray<int> ind = sp->GetRowIndices(i);
                                           FlatVector<double> rv = sp->GetRowValues(i);
                                           for (int j = 0; j < ind.Size(); j++)
                                             {
                                               ri.Append (i);
                                               ci.Append (ind[j]);
                                               vals.Append (rv[j]);
                                             }
                                         }

                                       bp::list pyri (ri);
                                       bp::list pyci (ci);
                                       bp::list pyvals (vals);
                                       return bp::make_tuple (pyri, pyci, pyvals);
                                     }
				   
				   SparseMatrix<Complex> * spc
				     = dynamic_cast<SparseMatrix<Complex>*> (&m);
				   if (spc)
				     {
					 Array<int> ri, ci;
					 Array<double> vals_real;
					 Array<double> vals_imag;
					 Array<Complex> vals;
					 for (int i = 0; i < spc->Height(); i++)
					   {
					     FlatArray<int> ind = spc->GetRowIndices(i);
					     FlatVector<Complex> rv = spc->GetRowValues(i);
					     for (int j = 0; j < ind.Size(); j++)
					       {
						 ri.Append (i);
						 ci.Append (ind[j]);
						 vals.Append (rv[j]);
					       }
					   }
					 bp::list pyri (ri);
					 bp::list pyci (ci);
					 bp::list pyvals(vals);
					 return bp::make_tuple (pyri, pyci, pyvals);
				     }
				   
				   throw Exception ("COO needs real or complex-valued sparse matrix");
                                 }))

    .def("Mult",        FunctionPointer( [](BM &m, BV &x, BV &y, double s) { m.Mult (x,y); y *= s; }) )
    .def("MultAdd",     FunctionPointer( [](BM &m, BV &x, BV &y, double s) { m.MultAdd (s, x, y); }))
    // .def("MultTrans",   FunctionPointer( [](BM &m, BV &x, BV &y, double s) { y  = s*Trans(m)*x; }) )
    // .def("MultTransAdd",FunctionPointer( [](BM &m, BV &x, BV &y, double s) { y += s*Trans(m)*x; }) )

    .add_property("expr", bp::object(expr_namespace["MatExpr"]) )
    .def("__mul__" , bp::object(expr_namespace["expr_mul"]) )
    .def("__rmul__" , bp::object(expr_namespace["expr_rmul"]) )

    .def("__iadd__", FunctionPointer( [] (BM &m, BM &m2) { 
        m.AsVector()+=m2.AsVector();
    }))

    .def("GetInverseType", FunctionPointer( [](BM & m)
                                            {
                                              return GetInverseName( m.GetInverseType());
                                            }))

    .def("Inverse", FunctionPointer( [](BM &m, BitArray * freedofs, string inverse)
                                     ->shared_ptr<BaseMatrix>
                                     { 
                                       if (inverse != "") m.SetInverseType(inverse);
                                       return m.InverseMatrix(freedofs);
                                     }),
         (bp::arg("self"), bp::arg("freedofs"), bp::arg("inverse")=""))
    .def("Inverse", FunctionPointer( [](BM &m)->shared_ptr<BaseMatrix>
                                     { return m.InverseMatrix(); }))
    .def("Transpose", FunctionPointer( [](BM &m)->shared_ptr<BaseMatrix>
                                       { return make_shared<Transpose> (m); }))
    .def("Update", FunctionPointer( [](BM &m) { m.Update(); }));
    // bp::return_value_policy<bp::manage_new_object>())
    ;

  REGISTER_PTR_TO_PYTHON_BOOST_1_60_FIX(shared_ptr<Projector>);
  bp::class_<Projector, shared_ptr<Projector>,bp::bases<BaseMatrix>,boost::noncopyable> ("Projector", bp::init<const BitArray&,bool>())
    ;

  REGISTER_PTR_TO_PYTHON_BOOST_1_60_FIX(shared_ptr<CGSolver<double>>);
  bp::class_<CGSolver<double>, shared_ptr<CGSolver<double>>,bp::bases<BaseMatrix>,boost::noncopyable> ("CGSolverD", bp::no_init)
    .def("GetSteps", &CGSolver<double>::GetSteps)
    ;

  REGISTER_PTR_TO_PYTHON_BOOST_1_60_FIX(shared_ptr<CGSolver<Complex>>);
  bp::class_<CGSolver<Complex>, shared_ptr<CGSolver<Complex>>,bp::bases<BaseMatrix>,boost::noncopyable> ("CGSolverC", bp::no_init)
    .def("GetSteps", &CGSolver<Complex>::GetSteps)
    ;

  bp::def("CGSolver", FunctionPointer ([](const BaseMatrix & mat, const BaseMatrix & pre,
                                          bool iscomplex, bool printrates, 
                                          double precision, int maxsteps) -> BaseMatrix *
                                       {
                                         KrylovSpaceSolver * solver;
                                         if(mat.IsComplex()) iscomplex = true;
                                         
                                         if (iscomplex)
                                           solver = new CGSolver<Complex> (mat, pre);
                                         else
                                           solver = new CGSolver<double> (mat, pre);
                                         solver->SetPrecision(precision);
                                         solver->SetMaxSteps(maxsteps);
                                         solver->SetPrintRates (printrates);
                                         return solver;
                                       }),
          (bp::arg("mat"), bp::arg("pre"), bp::arg("complex") = false, bp::arg("printrates")=true,
           bp::arg("precision")=1e-8, bp::arg("maxsteps")=200),
          bp::return_value_policy<bp::manage_new_object>()
          )
    ;

  REGISTER_PTR_TO_PYTHON_BOOST_1_60_FIX(shared_ptr<QMRSolver<double>>);
  bp::class_<QMRSolver<double>, shared_ptr<QMRSolver<double>>,bp::bases<BaseMatrix>,boost::noncopyable> ("QMRSolverD", bp::no_init)
    ;
  REGISTER_PTR_TO_PYTHON_BOOST_1_60_FIX(shared_ptr<QMRSolver<Complex>>);
  bp::class_<QMRSolver<Complex>, shared_ptr<QMRSolver<Complex>>,bp::bases<BaseMatrix>,boost::noncopyable> ("QMRSolverC", bp::no_init)
    ;
  bp::def("QMRSolver", FunctionPointer ([](const BaseMatrix & mat, const BaseMatrix & pre,
                                           bool printrates, 
                                           double precision, int maxsteps) -> BaseMatrix *
                                        {
                                          KrylovSpaceSolver * solver;
                                          if (!mat.IsComplex())
                                            solver = new QMRSolver<double> (mat, pre);
                                          else
                                            solver = new QMRSolver<Complex> (mat, pre);                                            
                                          solver->SetPrecision(precision);
                                          solver->SetMaxSteps(maxsteps);
                                          solver->SetPrintRates (printrates);
                                          return solver;
                                        }),
          (bp::arg("mat"), bp::arg("pre"), bp::arg("printrates")=true,
           bp::arg("precision")=1e-8, bp::arg("maxsteps")=200),
          bp::return_value_policy<bp::manage_new_object>()
          )
    ;


  REGISTER_PTR_TO_PYTHON_BOOST_1_60_FIX(shared_ptr<GMRESSolver<double>>);
  bp::class_<GMRESSolver<double>, shared_ptr<GMRESSolver<double>>,bp::bases<BaseMatrix>,boost::noncopyable> ("GMRESSolverD", bp::no_init)
    ;
  REGISTER_PTR_TO_PYTHON_BOOST_1_60_FIX(shared_ptr<GMRESSolver<Complex>>);
  bp::class_<GMRESSolver<Complex>, shared_ptr<GMRESSolver<Complex>>,bp::bases<BaseMatrix>,boost::noncopyable> ("GMRESSolverC", bp::no_init)
    ;
  bp::def("GMRESSolver", FunctionPointer ([](const BaseMatrix & mat, const BaseMatrix & pre,
                                           bool printrates, 
                                           double precision, int maxsteps) -> BaseMatrix *
                                        {
                                          KrylovSpaceSolver * solver;
                                          if (!mat.IsComplex())
                                            solver = new GMRESSolver<double> (mat, pre);
                                          else
                                            solver = new GMRESSolver<Complex> (mat, pre);                                            
                                          solver->SetPrecision(precision);
                                          solver->SetMaxSteps(maxsteps);
                                          solver->SetPrintRates (printrates);
                                          return solver;
                                        }),
          (bp::arg("mat"), bp::arg("pre"), bp::arg("printrates")=true,
           bp::arg("precision")=1e-8, bp::arg("maxsteps")=200),
          bp::return_value_policy<bp::manage_new_object>()
          )
    ;
  

  
  bp::def("ArnoldiSolver", FunctionPointer ([](BaseMatrix & mata, BaseMatrix & matm, const BitArray & freedofs,
                                               bp::list vecs, bp::object bpshift)
                                            {
                                              if (mata.IsComplex())
                                                {
                                                  Arnoldi<Complex> arnoldi (mata, matm, &freedofs);
                                                  Complex shift = 0.0;
                                                  if (bp::extract<Complex>(bpshift).check())
                                                    shift = bp::extract<Complex>(bpshift)();
                                                  cout << "shift = " << shift << endl;
                                                  arnoldi.SetShift (shift);
                                                  
                                                  int nev = bp::len(vecs);
                                                  cout << "num vecs: " << nev << endl;
                                                  Array<shared_ptr<BaseVector>> evecs(nev);
                                                  
                                                  Array<Complex> lam(nev);
                                                  arnoldi.Calc (2*nev+1, lam, nev, evecs, 0);
                                            
                                                  for (int i = 0; i < nev; i++)
                                                    * bp::extract<shared_ptr<BaseVector>>(vecs[i])() = *evecs[i];

                                                  Vector<Complex> vlam(nev);
                                                  for (int i = 0; i < nev; i++)
                                                    vlam(i) = lam[i];
                                                  return vlam;
                                                }
                                              
                                              cout << "real Arnoldi not supported" << endl;
                                              Vector<Complex> lam(5);
                                              return lam;
                                            }),
          (bp::arg("mata"), bp::arg("matm"), bp::arg("freedofs"), bp::arg("vecs"), bp::arg("shift")=bp::object())
          // bp::return_value_policy<bp::manage_new_object>()
          )
    ;

  

  bp::def("DoArchive" , FunctionPointer( [](shared_ptr<Archive> & arch, BaseMatrix & mat) 
                                         { cout << "output basematrix" << endl;
                                           mat.DoArchive(*arch); return arch; }));
                                           
}



void ExportNgbla();

BOOST_PYTHON_MODULE(libngla) {
  // ExportNgbla();
  ExportNgla();
}




#endif // NGS_PYTHON
