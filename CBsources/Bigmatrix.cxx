/* ****************************************************************************

    Copyright (C) 2004-2021  Christoph Helmberg

    ConicBundle, Version 1.a.2
    File:  CBsources/Bigmatrix.cxx
    This file is part of ConciBundle, a C/C++ library for convex optimization.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

***************************************************************************** */



#include <stdlib.h>
#include "mymath.hxx"
#include "Bigmatrix.hxx"
#include "sparssym.hxx"


using namespace CH_Matrix_Classes;

namespace ConicBundle {

  Bigmatrix::Bigmatrix() {
    tol = 1e-20;
    clear();
  }

  Bigmatrix::~Bigmatrix() {
    rowind.clear();
    rowval.clear();
  }

  void Bigmatrix::clear() {
    dim = nz = -1;
    nmult = 0;
    rowind.clear();
    rowval.clear();
    rowhash.clear();
    mcp.clear();
    mcv.init(0, 0, 0.);
    use_dense = false;
    symrep_init = false;
  }

  int  Bigmatrix::init(const Matrix& yin, Integer indim, const CoeffmatPointer C,
    const SparseCoeffmatVector* opA, const bool dense) {
    assert(indim >= 0);
    clear();

    //--- set dimension
    dim = indim;
    use_dense = false;

    //--- check for the dense case;
    if (dense) {
      use_dense = true;
      if (C != 0) {
        C->make_symmatrix(symrep);
        assert(dim == symrep.rowdim());
      } else symrep.init(dim, 0.);
      if (opA) {
        for (SparseCoeffmatVector::const_iterator mapi = (*opA).begin();
          mapi != (*opA).end(); ++mapi) {
          Integer i = (*mapi).first;
          CoeffmatPointer cp = (*mapi).second;
          if ((yin(i) == 0.) || (cp == 0)) continue;
          cp->addmeto(symrep, yin(i));
        }
      }
      symrep_init = true;
      return 0;
    }

    //--- collect indices and values of sparse matrices, store non-sparse info
    di.init(dim, 1, 0.);  //will hold all diagonal entries
    rowhash.resize((unsigned long)(dim));
    //if new elements are added in the hashmap these are assumed to be 
    //intialized to zero 

    //--- cost matrix
    if (C != 0) {
      if (C->sparse(collecti, collectj, collectval)) {
        Integer* cip = collecti.get_store();
        Integer* cjp = collectj.get_store();
        Real* cvp = collectval.get_store();
        for (Integer k = collecti.dim(); --k >= 0;) {
          if (*cip == *cjp) {
            di(*cip++) += *cvp++;
            cjp++;
          } else if (*cip < *cjp) {
            (*((rowhash[(unsigned long)*cip++].insert(Indexhash::value_type(*cjp++, 0.))).first)).second += *cvp++;
          } else {
            (*((rowhash[(unsigned long)*cjp++].insert(Indexhash::value_type(*cip++, 0.))).first)).second += *cvp++;
          }
        }
      } else {
        mcp.push_back(C);
        mcv.concat_below(1.);
      }
    }

    //--- coefficient matrices
    if (opA) {
      for (std::map<Integer, CoeffmatPointer>::const_iterator mapi = (*opA).begin();
        mapi != (*opA).end(); ++mapi) {
        Integer i = (*mapi).first;
        const CoeffmatPointer cp = (*mapi).second;
        if ((yin(i) == 0.) || (cp == 0)) continue;
        if (cp->sparse(collecti, collectj, collectval, yin(i))) {
          Integer* cip = collecti.get_store();
          Integer* cjp = collectj.get_store();
          Real* cvp = collectval.get_store();
          for (Integer k = collecti.dim(); --k >= 0;) {
            if (*cip == *cjp) {
              di(*cip++) += *cvp++;
              cjp++;
            } else if (*cip < *cjp) {
              (*((rowhash[(unsigned long)*cip++].insert(Indexhash::value_type(*cjp++, 0.))).first)).second += *cvp++;
            } else {
              (*((rowhash[(unsigned long)*cjp++].insert(Indexhash::value_type(*cip++, 0.))).first)).second += *cvp++;
            }
          }
        } else {
          mcp.push_back(cp);
          mcv.concat_below(yin(i));
        }
      }
    }

    //--- copy values in rowhash to rows
    nz = 0;
    colnz.newsize(dim, 1); chk_set_init(colnz, 1);
    rowind.resize((unsigned long)(dim));
    rowval.resize((unsigned long)(dim));

    for (Integer i = 0; i < dim; i++) {
      Integer sz = Integer(rowhash[(unsigned long)i].size());
      rowind[(unsigned long)(i)].newsize(sz, 1); chk_set_init(rowind[(unsigned long)(i)], 1);
      rowval[(unsigned long)(i)].newsize(sz, 1); chk_set_init(rowval[(unsigned long)(i)], 1);

      Indexhash::iterator del_it;
      Integer* ip = rowind[(unsigned long)(i)].get_store();
      Real* vp = rowval[(unsigned long)(i)].get_store();
      sz = 0;
      for (Indexhash::iterator it = rowhash[(unsigned long)i].begin(); it != rowhash[(unsigned long)i].end();) {
        Real d = it->second;
        if (d == 0.) {     //most likely this element has not been used at all
          Indexhash::iterator del_it = it;  //reduce the size of rowhash[i] for future iterations
          ++it;
          rowhash[(unsigned long)i].erase(del_it);
          continue;
        }
        if (fabs(d) > tol) {  //regard element as nonzero
          *ip++ = it->first - i;
          *vp++ = d;
          sz++;
        }
        it->second = 0.;
        ++it;
      }

      nz += sz;
      colnz[i] = sz;
      rowind[(unsigned long)(i)].reduce_length(sz);
      rowval[(unsigned long)(i)].reduce_length(sz);
      Indexmatrix tmpind;
      sortindex(rowind[(unsigned long)(i)], tmpind);
      rowind[(unsigned long)(i)] = (rowind[(unsigned long)(i)])(tmpind);
      rowval[(unsigned long)(i)] = (rowval[(unsigned long)(i)])(tmpind);
    }

    if ((dim == 0) || ((4 * nz) / dim >= dim)) {
      make_symmatrix(symrep);
      use_dense = true;
      symrep_init = true;
    }

    return 0;
  }

  Integer Bigmatrix::lanczosdim() const {
    return dim;
  }

  Integer Bigmatrix::lanczosflops() const {
    if (use_dense) return dim * dim;
    Integer flopcnt = 4 * nz;
    for (unsigned int i = 0; i < mcp.size(); i++) {
      flopcnt += mcp[i]->prodvec_flops();
    }
    return flopcnt;
  }


  int Bigmatrix::lanczosmult(const Matrix& A, Matrix& B) const {
    assert(dim >= 0);
    assert(dim == A.rowdim());
    if (use_dense) {
      genmult(symrep, A, B);
      nmult += A.coldim();
      return 0;
    }

    Integer nc = A.coldim();
    nmult += nc;
    if (nc >= 2) {
      //--- compute A transposed and initialize Bt=di*At for diagonal elements di
      At.newsize(nc, dim);
      Bt.newsize(nc, dim);
      for (Integer i = 0; i < nc; i++) {
        Real* abp = At.get_store() + i - nc;
        Real* bp = Bt.get_store() + i - nc;
        const Real* ap = A.get_store() + dim * i;
        const Real* ap2 = di.get_store();
        for (Integer j = dim; --j >= 0;) {
          *(bp += nc) = (*(abp += nc) = (*ap++)) * (*ap2++);
        }
      }
      chk_set_init(At, 1);
      chk_set_init(Bt, 1);

      //--- compute "Bt=bigm*At" for sparse offdiagonal part

      const Real* abp = At.get_store() + dim * nc;
      Real* bbp = Bt.get_store() + dim * nc;
      {
        for (Integer i = dim; --i >= 0;) { //for each column of the Sparsesym Matrix
          abp -= nc;
          bbp -= nc;
          Integer j = colnz[i];
          const Integer* hjp = rowind[(unsigned long)(i)].get_store() + j;
          const Real* mp = rowval[(unsigned long)(i)].get_store() + j;
          for (; --j >= 0;) {
            // offdiagonal entries only
            Real d = *(--mp);
            Real* bp = bbp;
            const Real* ap = abp + (*(--hjp)) * nc;
            for (Integer k = nc; --k >= 0;) {
              (*bp++) += d * (*ap++);
            }
            const Real* ap2 = abp;
            Real* bp2 = bbp + (*hjp) * nc;
            for (Integer k = nc; --k >= 0;) {
              (*bp2++) += d * (*ap2++);
            }
          }
        }
      }

      //--- store transposed Bt in B
      B.newsize(dim, nc);
      {
        for (Integer i = 0; i < nc; i++) {
          const Real* ap = Bt.get_store() + i - nc;
          Real* bp = B.get_store() + dim * i;
          for (Integer j = dim; --j >= 0;) {
            (*bp++) = *(ap += nc);
          }
        }
      }
      chk_set_init(B, 1);
    }

    else { //only one column: matrix times vector
      //--- compute "B=bigm*A" for sparse part
      //diagonal part
      B.newsize(dim, 1);
      const Real* abp = A.get_store();
      Real* bbp = B.get_store();
      const Real* mp = di.get_store();
      for (Integer i = dim; --i >= 0;) {
        (*bbp++) = (*abp++) * (*mp++);
      }
      chk_set_init(B, 1);

      //offdiagonal part
      abp = A.get_store() + dim;
      bbp = B.get_store() + dim;
      {
        for (Integer i = dim; --i >= 0;) { //for each column of the Sparsesym Matrix
          Integer j = colnz[i];
          --abp;
          Real abval = *abp;
          --bbp;
          Real bbval = *bbp;
          const Integer* hjp = rowind[(unsigned long)(i)].get_store() + j;
          const Real* mp = rowval[(unsigned long)(i)].get_store() + j;
          for (; --j >= 0;) {
            Real d;
            Integer h;
            bbval += (d = *(--mp)) * (*(abp + (h = *(--hjp))));
            *(bbp + h) += d * abval;
          }
          *bbp = bbval;
        }
      }
    }

    //--- add non-sparse constraints
    for (unsigned int i = 0; i < mcp.size(); i++) {
      mcp[i]->addprodto(B, A, mcv[Integer(i)]);
    }

    return 0;
  }

  int Bigmatrix::make_symmatrix(Symmatrix& S) const {
    assert(dim >= 0);
    if (symrep_init) {
      if (&S == &symrep)
        return 0;
      S = symrep;
      return 0;
    }
    S.init(dim, 0.);
    for (Integer i = 0; i < dim; i++) {
      S(i, i) += di(i);
      for (Integer j = 0; j < colnz[i]; j++) {
        S(i, rowind[(unsigned long)(i)][j] + i) += rowval[(unsigned long)(i)][j];
      }
    }
    {
      for (unsigned int i = 0; i < mcp.size(); i++)
        mcp[i]->addmeto(S, mcv[Integer(i)]);
    }
    return 0;
  }

  std::ostream& operator<<(std::ostream& out, const Bigmatrix& M) {
    if (M.mcp.size() == 0) {
      Integer lnz = 0;
      for (Integer i = 0; i < M.dim; i++) {
        if (M.di(i) != 0.) lnz++;
        lnz += M.colnz[i];
      }
      out << M.dim << " " << lnz << "\n"; out.precision(20);
      out.setf(std::ios::scientific | std::ios::left);
      {
        for (Integer i = 0; i < M.dim; i++) {
          if (M.di(i) != 0.)
            out << ' ' << i << ' ' << i << ' ' << M.di(i) << '\n';
          for (Integer j = 0; j < M.colnz[i]; j++) {
            out << ' ' << i << ' ' << M.rowind[(unsigned long)(i)][j] + i << ' ' << M.rowval[(unsigned long)(i)][j] << '\n';
          }
        }
      }
    } else {
      Integer lnz = 0;
      static Symmatrix S;
      M.make_symmatrix(S);
      for (Integer i = 0; i < M.dim; i++) {
        for (Integer j = i; j < M.dim; j++) {
          if (fabs(S(i, j)) >= M.tol) lnz++;
        }
      }
      out << M.dim << " " << lnz << "\n"; out.precision(20);
      out.setf(std::ios::scientific | std::ios::left);
      {
        for (Integer i = 0; i < M.dim; i++) {
          for (Integer j = i; j < M.dim; j++) {
            if (fabs(S(i, j)) < M.tol) continue;
            out << ' ' << i << ' ' << j << ' ' << S(i, j) << '\n';
          }
        }
      }
    }
    return out;
  }

}

