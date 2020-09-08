import ngsolve as ngs
import petsc4py.PETSc as psc
from mpi4py import MPI
import numpy as np

def CreatePETScMatrix (ngs_mat, freedofs=None):
    pardofs = ngs_mat.row_pardofs
    comm = pardofs.comm.mpi4py
    globnums, nglob = pardofs.EnumerateGlobally(freedofs)
    if freedofs is not None:
        globnums = np.array(globnums, dtype="int32")[freedofs]
        
    iset = psc.IS().createGeneral (indices=globnums, comm=comm)
    lgmap = psc.LGMap().createIS(iset)
    
    locmat = ngs_mat.local_mat
    val,col,ind = locmat.CSR()
    ind = np.array(ind, dtype='int32')
    apsc_loc = psc.Mat().createAIJ(size=(locmat.height, locmat.width), csr=(ind,col,val), comm=MPI.COMM_SELF)

    if freedofs is not None:
        locfree = [i for i,b in enumerate(freedofs) if b]
        isfree_loc = psc.IS().createGeneral(indices=locfree)
        apsc_loc = apsc_loc.createSubMatrices(isfree_loc)[0]
    
    mat = psc.Mat().createPython(size=nglob, comm=comm)
    mat.setType(psc.Mat.Type.IS)
    mat.setLGMap(lgmap)
    mat.setISLocalMat(apsc_loc)
    mat.assemble()
    mat.convert("mpiaij")
    mat.view()
    return mat



class VectorMapping:
    def __init__ (self, pardofs, freedofs=None):
        self.pardofs = pardofs
        self.freedofs = freedofs
        comm = pardofs.comm.mpi4py        
        globnums, nglob = pardofs.EnumerateGlobally(freedofs)
        if self.freedofs is not None:
            globnums = np.array(globnums, dtype="int32")[freedofs]
            self.locfree = [i for i,b in enumerate(freedofs) if b]
            # self.isfree_loc = psc.IS().createGeneral(indices=self.locfree)
        
        self.iset = psc.IS().createGeneral (indices=globnums, comm=comm)

    def CreatePETScVector (self):
        return psc.Vec().createMPI(self.pardofs.ndofglobal, comm=MPI.COMM_WORLD)        

    def CreateNGSolveVector (self):
        return ngs.la.CreateParallelVector(self.pardofs)
    
    def N2P (self, ngs_vector, psc_vector=None):
        if psc_vector is None:
            psc_vector = psc.Vec().createMPI(self.pardofs.ndofglobal, comm=MPI.COMM_WORLD)
        ngs_vector.Cumulate()
        psc_loc = psc_vector.getSubVector(self.iset)
        if self.freedofs is None:
            psc_loc.getArray()[:] = ngs_vector.FV()
        else:
            psc_loc.getArray()[:] = ngs_vector.FV().NumPy()[self.locfree]
        
        psc_vector.restoreSubVector(self.iset, psc_loc)
        return psc_vector

    def P2N (self, psc_vector, ngs_vector=None):
        if ngs_vector is None:
            ngs_vector = ngs.la.CreateParallelVector(self.pardofs)
        psc_loc = psc_vector.getSubVector(self.iset)

        if self.freedofs is None:
            ngs_vector.FV()[:] = ngs.Vector(psc_loc.getArray())
        else:
            ngs_vector.FV().NumPy()[:] = 0
            ngs_vector.FV().NumPy()[self.locfree] = ngs.Vector(psc_loc.getArray())            
        return ngs_vector
    


    
class PETScPreconditioner(ngs.BaseMatrix):
    def __init__(self,mat,freedofs=None):
        ngs.BaseMatrix.__init__(self)
        self.ngsmat = mat
        self.vecmap = VectorMapping (mat.row_pardofs, freedofs)
        self.mat = CreatePETScMatrix (mat, freedofs)

        self.precond = psc.PC().create()
        self.precond.setType("gamg")
        self.precond.setOperators(self.mat)
        self.pscx, self.pscy = self.mat.createVecs()
        
    def Height(self):
        return self.ngsmat.height
    def Width(self):
        return self.ngsmat.width
    
    def Mult(self, x,y):
        self.vecmap.N2P(x,self.pscx)
        self.precond.apply(self.pscx, self.pscy)
        self.vecmap.P2N(self.pscy, y)
        

def MakePreconditioner(mat, freedofs):
    return PETScPreconditioner(mat, freedofs)

from ngsolve.comp import RegisterPreconditioner
RegisterPreconditioner ("gamg", MakePreconditioner)


    