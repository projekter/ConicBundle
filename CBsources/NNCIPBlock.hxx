/* ****************************************************************************

    Copyright (C) 2004-2021  Christoph Helmberg

    ConicBundle, Version 1.a.2
    File:  CBsources/NNCIPBlock.hxx
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



#ifndef CONICBUNDLE_NNCIPBLOCK_HXX
#define CONICBUNDLE_NNCIPBLOCK_HXX

/**  @file NNCIPBlock.hxx
    @brief Header declaring the class ConicBundle::NNCIPBlock
    @version 1.0
    @date 2020-05-02
    @author Christoph Helmberg
*/

#include "InteriorPointBlock.hxx"

namespace ConicBundle {

  /** @ingroup ConstrainedQPSolver
   */

   //@{

   /** @brief  interface for the interior point variable vector and routines specific to the primal dual complementarity conditions of the nonnegative cone

   */

  class NNCIPBlock : public virtual InteriorPointBlock {
  protected:
    CH_Matrix_Classes::Integer vecdim;  ///< the dimension of the cone
    CH_Matrix_Classes::Matrix x;     ///< "primal" point x
    CH_Matrix_Classes::Matrix z;     ///< "dual" point z
    CH_Matrix_Classes::Matrix dx;    ///< current step for x
    CH_Matrix_Classes::Matrix dz;    ///< current step for z

    CH_Matrix_Classes::Matrix xiz;        ///< z(i)/x(i)
    CH_Matrix_Classes::Matrix compl_rhs;  ///< rhs used in solving the complementarity line

    CH_Matrix_Classes::Real last_rhs_mu;  ///< the last mu used in rhs computations
    CH_Matrix_Classes::Real mu;           ///< in a step mu gets the value of last_rhs_mu
    CH_Matrix_Classes::Real old_mu;       ///< in a step old_mu gets the value of mu before this gets last_rhs_mu

    CH_Matrix_Classes::Real last_alpha;   ///< last alpha used in do_step()

    CH_Matrix_Classes::Matrix oldx;       ///< point before x
    CH_Matrix_Classes::Matrix oldz;       ///< point before z

    CH_Matrix_Classes::Matrix tmpvec;    ///< temporary vector to reduce reallocations
    CH_Matrix_Classes::Matrix tmpmat;    ///< temporary matrix to reduce reallocatoins

    /// clear variables that are no longer valid for the current point
    void point_changed();

    /// here this just computes xiz=z/x in a separate routine in order to have uniform appearance in the other subroutines
    inline int compute_NTscaling(void) {
      xiz.init(z);
      xiz /= x;

      //TEST
      // CH_Matrix_Classes::Matrix tmpvec(x);
      // tmpvec.concat_right(z);
      // tmpvec.concat_right(xiz);
      // tmpvec.concat_right(inv(xiz));
      // std::cout<<" NNCIP "; tmpvec.display(std::cout);

      return 0;
    }

  public:
    /// reset all point information to zero for dimension dim, the rest to zero
    virtual void clear(CH_Matrix_Classes::Integer dim = 0);

    /// default constructor, also allows to initialize the dimension
    NNCIPBlock(CH_Matrix_Classes::Integer dim = 0, CBout* cb = 0, int cbinc = -1);

    /// destructor
    ~NNCIPBlock();

    ///returns the dimension of the cone
    virtual CH_Matrix_Classes::Integer get_vecdim() const;

    /// set x to value*"one" to x, or if add==true, add value*"one" to x
    virtual int center_x(CH_Matrix_Classes::Real val, bool add = false);

    /// set z to value*"one" to z, or if add==true, add value*"one" to z
    virtual int center_z(CH_Matrix_Classes::Real val, bool add = false);

    /// set x to the values of vec[startindex+0,+1 ...,+(vecdim-1)] and return in add_center_value a value>=0 that needs to be added to make it feasible
    virtual int set_x(const CH_Matrix_Classes::Matrix& vec, CH_Matrix_Classes::Integer startindex, CH_Matrix_Classes::Real& add_center_value);

    /// set z to the values of vec[startindex+0,+1 ...,+(vecdim-1)] and add sufficient center to make z feasible, return this value>=0 in added_center_value
    virtual int set_z(const CH_Matrix_Classes::Matrix& vec, CH_Matrix_Classes::Integer startindex, CH_Matrix_Classes::Real& add_center_value);

    /// on vec[startindex+0,+1 ...,+(vecdim-1)] put or add  a * x into vec for a real number a  
    virtual int vecgetsax(CH_Matrix_Classes::Matrix& vec,
      CH_Matrix_Classes::Integer startindex,
      CH_Matrix_Classes::Real a = 1.,
      bool add = false);

    /// on vec[startindex+0,+1 ...,+(vecdim-1)] put or add a * z into vec for a real number a   
    virtual int vecgetsaz(CH_Matrix_Classes::Matrix& vec,
      CH_Matrix_Classes::Integer startindex,
      CH_Matrix_Classes::Real a = 1.,
      bool add = false);

    /// add dimensions of the primal-dual pairs to mudim and add the "trace" (the inner product with center) of the respective primal-dual pair products for the current step; update the min and max values of x_i*z_i
    virtual int get_mu_info(CH_Matrix_Classes::Integer& mudim,
      CH_Matrix_Classes::Real& tr_xz,
      CH_Matrix_Classes::Real& tr_xdzpdxz,
      CH_Matrix_Classes::Real& tr_dxdz,
      CH_Matrix_Classes::Real& min_xz,
      CH_Matrix_Classes::Real& max_xz) const;

    /// for limiting the stepsize with respect to the neighborhood this information about norms and inner products of x(.)*z-tr_xz-tr_xz/mudim(.*)1, x.()*dz+dx(.)*z-tr_xdzpdxz/mudim(.*)1, and dx(.)*dz-tr_dxdz/mudim(.)*1 is required, each block *adds* its contribution to the numbers
    virtual int get_nbh_info(CH_Matrix_Classes::Integer mudim,
      CH_Matrix_Classes::Real tr_xz,
      CH_Matrix_Classes::Real tr_xdzpdxz,
      CH_Matrix_Classes::Real tr_dxdz,
      CH_Matrix_Classes::Real nbh_ubnd,
      CH_Matrix_Classes::Real& alpha,
      CH_Matrix_Classes::Real& max_nbh,
      CH_Matrix_Classes::Real& nrmsqr_xz,
      CH_Matrix_Classes::Real& nrmsqr_xdzpdxz,
      CH_Matrix_Classes::Real& nrmsqr_dxdz,
      CH_Matrix_Classes::Real& ip_xz_xdzpdxz,
      CH_Matrix_Classes::Real& ip_xz_dxdz,
      CH_Matrix_Classes::Real& ip_dxdz_xdzpdxz) const;

    /// if necessary, reduce alpha to the biggest value so that feasibility is maintained with this step size
    virtual int linesearch(CH_Matrix_Classes::Real& alpha) const;

    /// compute the complementarity_rhs=rhsmu*xi-rhscorr*xi*dx*dz (wihtout "-z") for mu=rhsmu and for corrector for factor rhscorr>0., store this and add it to rhs 
    virtual int add_muxinv(CH_Matrix_Classes::Matrix& rhs,
      CH_Matrix_Classes::Integer startindex,
      CH_Matrix_Classes::Real rhsmu,
      CH_Matrix_Classes::Real rhscorr,
      bool minus = false);


    /// extract dx from rhs at startindex and compute at the same time dz (=-sys dx-z  +complentarity_rhs); 
    virtual int set_dx(const CH_Matrix_Classes::Matrix& rhs, CH_Matrix_Classes::Integer startindex);

    /// compute dx=sysinv*rhs and at the same time dz (=-rhs -z +complentarity_rhs); 
    virtual int set_dx_xizsolverhs(const CH_Matrix_Classes::Matrix& rhs, CH_Matrix_Classes::Integer startindex);

    /// compute sysinv*rhs into rhs, possibly with a negative sign 
    virtual int apply_xizinv(CH_Matrix_Classes::Matrix& rhs,
      CH_Matrix_Classes::Integer startindex,
      bool minus = false);

    /// compute sys*rhs into rhs, possibly with a negative sign
    virtual int apply_xiz(CH_Matrix_Classes::Matrix& rhs,
      CH_Matrix_Classes::Integer startindex,
      bool minus = false);

    /// move to (x+alpha*dx, z+alpha*dz)
    virtual int do_step(CH_Matrix_Classes::Real alpha);

    /// add the Schur complement to a big system matrix
    virtual int add_AxizinvAt(const CH_Matrix_Classes::Matrix& A,
      CH_Matrix_Classes::Symmatrix& globalsys,
      bool minus = false,
      bool Atrans = false);

    /// add (or subract if minus==true) the system matrix to a big system matrix starting at startindex
    virtual int add_xiz(CH_Matrix_Classes::Symmatrix& globalsys, CH_Matrix_Classes::Integer startindex, bool minus = false);



    //---------------------- mainly for testing

    /// return the vector form of x 
    virtual int get_vecx(CH_Matrix_Classes::Matrix& vecx, CH_Matrix_Classes::Integer startindex) {
      CH_Matrix_Classes::mat_xey(vecdim, vecx.get_store() + startindex, x.get_store()); return 0;
    }

    /// return the vector form of z
    virtual int get_vecz(CH_Matrix_Classes::Matrix& vecz, CH_Matrix_Classes::Integer startindex) {
      CH_Matrix_Classes::mat_xey(vecdim, vecz.get_store() + startindex, z.get_store()); return 0;
    }

    /// return the vector form of dx, 1 if not available 
    virtual int get_vecdx(CH_Matrix_Classes::Matrix& vecdx, CH_Matrix_Classes::Integer startindex) {
      if (dx.dim() != vecdim)
        return 1;
      CH_Matrix_Classes::mat_xey(vecdim, vecdx.get_store() + startindex, dx.get_store());
      return 0;
    }


    /// return the vector form of dz, 1 if not available
    virtual int get_vecdz(CH_Matrix_Classes::Matrix& vecdz, CH_Matrix_Classes::Integer startindex) {
      if (dz.dim() != vecdim)
        return 1;
      CH_Matrix_Classes::mat_xey(vecdim, vecdz.get_store() + startindex, dz.get_store());
      return 0;
    }

  };


  //@}

}

#endif

