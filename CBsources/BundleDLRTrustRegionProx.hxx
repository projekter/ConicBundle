/* ****************************************************************************

    Copyright (C) 2004-2021  Christoph Helmberg

    ConicBundle, Version 1.a.2
    File:  CBsources/BundleDLRTrustRegionProx.hxx
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



#ifndef CONICBUNDLE_DLRTRUSTREGIONPROX_HXX
#define CONICBUNDLE_DLRTRUSTREGIONPROX_HXX


/**  @file BundleDLRTrustRegionProx.hxx
    @brief Header declaring the class ConicBundle::BundleDLRTrustRegionProx
    @version 1.0
    @date 2020-03-12
    @author Christoph Helmberg
*/


#include "BundleProxObject.hxx"

namespace ConicBundle {

  /**@ingroup InternalBundleProxObject
   */

   //@{

  /** @brief implements the abstract interface ConicBundle::BundleProxObject for \f$\|y-\hat{y}\|_H^2\f$
      with \f$ H=VV^\top+D+u I\f$, i.e., a low rank representation of a symmetric positive definite matrix with a diagonal and the weight times identity added as regularization

  */

  class BundleDLRTrustRegionProx : public BundleProxObject {
  private:
    /// the current weight added to H in the form of weightu*I
    CH_Matrix_Classes::Real weightu;

    /// the diagonal with weightu in it
    CH_Matrix_Classes::Matrix D;

    /// orthogonal columns for the low rank representation (columns scaled by sqrts of eigenvalues)
    CH_Matrix_Classes::Matrix vecH;

    /// the inverse diagonal
    mutable CH_Matrix_Classes::Matrix Dinv;

    /// this holds L^{-1}*vecH^T*D^{-1} whenever its column dimension equals the row dimension of vecH 
    mutable CH_Matrix_Classes::Matrix LinvHt;

    /// the correction value for correcting termination precision
    CH_Matrix_Classes::Real corr_val;

    /// the fixed indices for which the QP_costs were computed 
    CH_Matrix_Classes::Indexmatrix old_fixed_ind;
    /// for compute_QP_costs() this holds the subgradients with zeroed fixed parts 
    CH_Matrix_Classes::Matrix _A;
    /// for compute_QP_costs() this holds the constant subgradient with zeroed fixed parts
    CH_Matrix_Classes::Matrix _b;
    /// for compute_QP_costs() this holds the offset values including fixed contributions
    CH_Matrix_Classes::Matrix _c;
    /// for compute_QP_costs() this holds the constant offset including fixed contributions
    CH_Matrix_Classes::Real _delta;
    /// for compute_QP_costs() this holds the non_fixed part of center_y
    CH_Matrix_Classes::Matrix _y;
    /// the inverse diagonal
    mutable CH_Matrix_Classes::Matrix indDinv;
    /// for compute_QP_costs() this holds L^{-1}* vecH^T for non fixed indices 
    mutable CH_Matrix_Classes::Matrix indLinvHt;
    /// the old quadratic cost matrix dependent on the low rank part 
    CH_Matrix_Classes::Symmatrix old_LinvQ;
    /// the old linear cost term; 
    CH_Matrix_Classes::Matrix oldd;
    /// the old costant cost term; 
    CH_Matrix_Classes::Real oldoffset;

    /// if values should be computed for a new subset indices, this is stored here
    const CH_Matrix_Classes::Indexmatrix* new_indices;

    /// in add_variable_metric() the stack serves to transform the given data
    std::vector<const AffineFunctionTransformation*> aft_stack;

    /// flag for variable metric, whether columns have been orthogonalized and lam truncated
    bool needs_cleaning;

    /// for variable metric this gives an upper bound for collecting columns before cleaning
    CH_Matrix_Classes::Integer max_columns;

    /// if not yet available this computes the low rank inverse data (possibly restricted to non fixed indices) 
    void provide_inverse_data(bool skip_old_fixed_ind = false) const;

    /// if the metric or weightu changes, this resets the inverse data to "not provided"
    void clear_inverse_data();

    /// if not clean this orthogonalizes vecH and trucates lamH
    void clean();

    /// computes the correction value, here min(1,dim/trace(H))
    void compute_corr()
      //{ corr_val=sum(D)+CH_Matrix_Classes::sqr(norm2(vecH))-weightu*D.dim(); if (corr_val>D.dim()) corr_val=D.dim()/corr_val; else corr_val=1.;}
    {
      corr_val = CH_Matrix_Classes::min(1., D.rowdim() / (sum(D) + CH_Matrix_Classes::sqr(norm2(vecH))));
    }

  public:
    /// default constructor with empty H (equal to zero) and the dimension as argument
    BundleDLRTrustRegionProx(CH_Matrix_Classes::Integer dim = 0,
      VariableMetricSelection* vp = 0,
      bool local_metric = false,
      CBout* cb = 0, int inc = -1) :
      BundleProxObject(vp, local_metric, false, cb, inc),
      weightu(1.), D(dim, 1, weightu), vecH(dim, 0, 0.) {
      assert(dim >= 0);
    }

    /// constructs H=(D+weightu)+vecH*transpose(vecH) with weightu=1., so in_D must be a column vector with same dimension as rows in in_vecH
    BundleDLRTrustRegionProx(const CH_Matrix_Classes::Matrix& in_D,
      const CH_Matrix_Classes::Matrix& in_vecH,
      VariableMetricSelection* vp = 0,
      bool local_metric = false,
      CBout* cb = 0, int inc = -1) :
      BundleProxObject(vp, local_metric, false, cb, inc) {
      weightu = 1.; init(in_D, in_vecH);
    }

    /// 
    virtual ~BundleDLRTrustRegionProx() {
    }

    /// sets the next weight
    void set_weightu(CH_Matrix_Classes::Real in_weightu);

    /// returns the current weight in use
    CH_Matrix_Classes::Real get_weightu() const {
      return weightu;
    }

    ///returns a correction factor for termination precision if the quadratic term is strong
    CH_Matrix_Classes::Real get_term_corr() const {
      return corr_val;
    }

    /// reset the prox information; the diagaonal part in_D must be a nonnegative column vector, the low rank part in_vecH must have the same number of rows as in_D 
    void init(const CH_Matrix_Classes::Matrix& in_D,
      const CH_Matrix_Classes::Matrix& in_vecH);

    /// returns $\|B\|^2_H$ (with weight included)
    virtual CH_Matrix_Classes::Real norm_sqr(const CH_Matrix_Classes::Matrix& B) const;

    /// returns $\|B\|^2_{H^{-1}}$ (with weight included)
    virtual CH_Matrix_Classes::Real dnorm_sqr(const MinorantPointer& B) const;

    ///return true if H is of the form diagonal matrix plus Gram matrix of a low rank matrix
    virtual bool is_DLR() const {
      return true;
    }

    /// add H to the dense symmetric matrix as a principal submatrix starting at position start_index
    virtual int add_H(CH_Matrix_Classes::Symmatrix& big_sym, CH_Matrix_Classes::Integer start_index = 0) const;

    /// adds \f$alpha*Hx\f$ to outplusHx and returns this
    virtual CH_Matrix_Classes::Matrix& add_Hx(const CH_Matrix_Classes::Matrix& x,
      CH_Matrix_Classes::Matrix& outplusHx,
      CH_Matrix_Classes::Real alpha = 1.) const;

    /// returns \f$H^{-1}x\f$ where \f$H^{-1}=\frac1u(I-V(\Lambda/(\Lambda+u))V^\top)\f$
    virtual CH_Matrix_Classes::Matrix& apply_Hinv(CH_Matrix_Classes::Matrix& x) const;

    /// returns a suitable approximation for preconditioning, see BundleProxObject::get_precond 
    virtual void get_precond(CH_Matrix_Classes::Matrix& inD, const CH_Matrix_Classes::Matrix*& Vp) const;

    /// computes the dual QP costs Q, d, and the constant offset to the bundle subproblem, see BundleProxObject::compute_QP_costs
    virtual int compute_QP_costs(CH_Matrix_Classes::Symmatrix& Q,
      CH_Matrix_Classes::Matrix& d,
      CH_Matrix_Classes::Real& offset,
      const MinorantPointer& constant_minorant,
      const MinorantBundle& bundle,
      const CH_Matrix_Classes::Matrix& y,
      const MinorantPointer& groundset_minorant,
      CH_Matrix_Classes::Indexmatrix* yfixed);


    /// updates the dual QP costs Q, d, and the constant offset to the bundle subproblem, see BundleProxObject::update_QP_costs
    virtual int update_QP_costs(CH_Matrix_Classes::Symmatrix& delta_Q,
      CH_Matrix_Classes::Matrix& delta_d,
      CH_Matrix_Classes::Real& delta_offset,
      const MinorantPointer& constant_minorant,
      const MinorantBundle& bundle,
      const CH_Matrix_Classes::Matrix& center_y,
      const MinorantPointer& groundset_minorant,
      const MinorantPointer& delta_groundset_minorant,
      const CH_Matrix_Classes::Indexmatrix& delta_index,
      CH_Matrix_Classes::Indexmatrix* yfixed);


    /// when BundleSolver is called to modify the groundset it also calls this 
    virtual int apply_modification(const GroundsetModification& gsmdf);


    /** @brief in order to allow for fixed variables, this generates a clone restricted to the given indices
   */
    virtual BundleProxObject* projected_clone(const CH_Matrix_Classes::Indexmatrix& indices);

    /** @brief this implementation does not support a diagonal scaling heuristic,
        therefore the following routine has to return true.
     */
    virtual bool supports_diagonal_bounds_scaling() const {
      return false;
    }

    /** @brief if supported, D_update has to contain nonnegative numbers that
        are permanently added to the diagonal here. It is important to keep
        track of this change only if afterwards update_QP_costs is called before
        compute_QP_costs. In this case the only nonzero enries in D_update must
        be those of delta_index
     */
    virtual int diagonal_bounds_scaling_update(const CH_Matrix_Classes::Matrix& /* D_update */) {
      return 1;
    }

    /// returns true if variable metric with dense symmetric matrices is supported
    virtual bool supports_dense_variable_metric() const {
      return false;
    }

    /// returns true if variable metric with low rank structure is supported
    virtual bool supports_lowrank_variable_metric() const {
      return true;
    }

    /// returns true if variable metric with diagonal matrices is supported
    virtual bool supports_diagonal_variable_metric() const {
      return true;
    }

    /// see DynamicScaling
    virtual int apply_variable_metric(VariableMetricModel* groundset,
      VariableMetricModel* model,
      const CH_Matrix_Classes::Matrix& aggr,
      CH_Matrix_Classes::Integer y_id,
      const CH_Matrix_Classes::Matrix& y,
      bool descent_step,
      CH_Matrix_Classes::Real& current_weight,
      CH_Matrix_Classes::Real model_maxviol,
      const CH_Matrix_Classes::Indexmatrix* new_indices = 0);

    /// see BundleProxObject::add_variable_metric()
    virtual int add_variable_metric(CH_Matrix_Classes::Matrix& diagH,
      CH_Matrix_Classes::Matrix& vecH);

    /// see  BundleProxObject::push_aft();
    virtual int push_aft(const AffineFunctionTransformation* aft) {
      aft_stack.push_back(aft); return 0;
    }

    /// see  BundleProxObject::pop_aft();
    virtual int pop_aft() {
      aft_stack.pop_back();
      return 0;
    }

    /// output the description of the scaling in mfile-suitable format
    virtual int mfile_data(std::ostream& out) const;


  };


  //@}
}

#endif

