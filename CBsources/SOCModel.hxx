/* ****************************************************************************

    Copyright (C) 2004-2021  Christoph Helmberg

    ConicBundle, Version 1.a.2
    File:  CBsources/SOCModel.hxx
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



#ifndef CONICBUNDLE_SOCMODEL_HXX
#define CONICBUNDLE_SOCMODEL_HXX

/**  @file SOCModel.hxx
    @brief Header declaring the class ConicBundle::SOCModel
    @version 1.0
    @date 2017-09-28
    @author Christoph Helmberg
*/

#include "ConeModel.hxx"
#include "SOCData.hxx"
#include "SOCModelParametersObject.hxx"

namespace ConicBundle {

  /** @ingroup InternalBundleModel
  */
  //@{

  /**@brief implements (as a derived class of ConeModel) a cutting model for a SOCOracle

     the rountines in this model implement a second order cutting model
     like in the spectral bundle method for the convex function

     \f[ \gamma \max_{x\in\{\left(\begin{array}{c}1\\\bar x\end{array}\right)\colon \|\bar x\|\le 1}(c+\sum A_iy_i)^\top x \f]

     where the data is given in abstract form by a ConicBundle::SOCOracle and
     in a concrete implmentation by ConicBundle::AffineSOCFunction as follows
     - c is the constant offset vector of SOCOracle
     - A_i are the cofficient matrices as described by opAt of SOCOracle
     - \f$\gamma\f$ is the (fixed/adaptive, see ConicBundle::FunctionTask)
       multiplier of the second order cone constraint specified on
       initialization, e.g. in ConicBundle::MatrixCBSolver::add_function()


     The bundle data is stored in ConicBundle::SOCData and represents a face of the
     second order cone possibly together with some aggregate vecotrs.
   */


  class SOCModel : public ConeModel {
  private:

    //--- problem description
    /// points to the SOCOracle for which it implements the cutting model
    SOCOracle* oracle;
    //--- data describing the model
    /// the relevant data for running BundleSolver consistently
    SOCData data;
    /// parameters for steering size and update strategy of the cutting plane model
    SOCModelParametersObject* model_selection;

    //--- data of last oracle evaluation
    /// return value of the last call to MatrixFunctionOracle::evaluate()
    int ret_code;
    /// total number of oralce calls (to  MatrixFunctionOracle::evaluate())
    CH_Matrix_Classes::Integer nr_eval;

    //--- augmented modle solver
    /// describes the feasible convex combinations of the bundle vectors for the bundle subproblem
    QPConeModelDataObject* block;


    //--- for debugging
    //CH_Matrix_Classes::Matrix debug_cand_y;
    //CH_Matrix_Classes::Matrix debug_center_y;

     //----------------------------------------------------------------------
    /** @name private methods for cleaning up bundle data and lazy computation of minorants */
    //@{ 

    /// Compute the aggregate minornat corresponding to the matrix P*Diag(d)*P'/sum(d) and aggregate it to (or, if empty or mp_coeff=0., store it in) mp; P has orthonormal columns and d is a nonengative vector of appropriate size.
    int get_minorant(MinorantPointer& mp,
      const CH_Matrix_Classes::Matrix& socvec);

    //@}


     //----------------------------------------------------------------------
    /** @name implemented methods required by ConeModel  */
    //@{ 


    /// see SumBlockModel::eval_function() but ignore ub_fid and ub with function_factor and function_task, this is set outside. Set the data.cand* information and in particular data.cand_ub_mid to data.get_modification_id() if the evaluation results make some sense even in the presence of errors. Please measure the oracle's SumBlockModel::eval_time as well as SumBlockModel::preeval_time and SumBlockModel::posteval_time here if this information is of interest
    virtual int evaluate_oracle(CH_Matrix_Classes::Integer y_id,
      const CH_Matrix_Classes::Matrix& y,
      CH_Matrix_Classes::Real nullstep_bound,
      CH_Matrix_Classes::Real relprec);

    /// see SumBlockModel::provide_model_aggregate(), only called if local model is active
    virtual int provide_local_aggregate(CH_Matrix_Classes::Integer y_id,
      const CH_Matrix_Classes::Matrix& iny);

    /// see SumBlockModel::eval_model() but ignore function_factor and function_task, this is set by ConeModel::eval_model(); note: evalmodel_time is already kept by ConeModel
    virtual int evaluate_local_model(CH_Matrix_Classes::Real& lb,
      CH_Matrix_Classes::Integer y_id,
      const CH_Matrix_Classes::Matrix& y,
      CH_Matrix_Classes::Real relprec);

    /// see SumBlockModel::update_model(); it is called irrespective of whether the local model is active or not; in particular, data.do_step() has to be called here if model_update==descent_step (this has the purpose to allow the local model to use the data of the old center before it is overwritten by the current candidate); model_maxviol and H are as passed to ConeModel by SumBlockModel::update_model(); note: updatemodel_time is already kept by ConeModel
    virtual int update_local_model(ModelUpdate model_update,
      CH_Matrix_Classes::Integer center_id,
      const CH_Matrix_Classes::Matrix& center_y,
      CH_Matrix_Classes::Integer y_id,
      const CH_Matrix_Classes::Matrix& y,
      CH_Matrix_Classes::Real model_maxviol,
      BundleProxObject& H);

    /// the local model has to take over after sumbundle was active; current_model holds the final model and coefficients so that the local model can take over whatever it needs, in particualr the aggregate
    virtual int switch_to_local(const MinorantBundle& sumbundle_model,
      const CH_Matrix_Classes::Matrix& sumbundle_coeff);

    /// the sumbundle is initialized (not necessarily switched on) and needs to know the normalized aggregate of the local model without the influence of the function_factor as well as the coefficient reflecting the influnce of the function_factor; this is called in alternative to start_local_augmodel, so some information has to be available
    virtual int get_aggregate_and_coefficient(MinorantPointer& sumbundle_aggr,
      CH_Matrix_Classes::Real& aggr_coeff);



    /// see SumBlockModel::start_augmodel(); the local QP block is exclusively organized here; the routine is only called, if the local modle is active
    virtual int start_local_augmodel(QPModelDataPointer& qpblockp,
      CH_Matrix_Classes::Integer cand_id,
      const CH_Matrix_Classes::Matrix& cand_y,
      const CH_Matrix_Classes::Indexmatrix* needed_indexsubset);

    /// see SumBlockModel::make_model_aggregate for the general rationale; the routine is only called, if the local modle is active; if the local model decides to increase the function factor, it reports back this value in new_function_factor (only allowed if keep_penalty_fixed==false); the new data.function_factor and the modification ids are the set in ConeModel
    virtual int make_local_aggregate(CH_Matrix_Classes::Real& new_function_factor,
      bool keep_penalty_fixed);

    /// see SumBlockModel::adjust_multiplier();  the routine is only called for adaptive penalty functions and if the local model is active; if the local model decides to adapt the function factor (here it is typically decreased), it reports back this value in new_function_factor; the new data.function_factor and the modification ids are the set in ConeModel
    virtual int local_adjust_multiplier(CH_Matrix_Classes::Real& new_function_factor);

    /// returns true if candidate information (always the results of the most recent function evaulation) is available and up to date (no model changes in the meantime) 
    virtual bool candidate_available() {
      return (data.cand_ub_mid == data.get_modification_id()) && (data.cand_SOCvec.coldim() > 0);
    }

    /// if it cannot be provided it returns a minorant with valid()==false
    virtual MinorantPointer& provide_cand_minorant();

    /// if it cannot be provided it returns a minorant with valid()==false
    virtual MinorantPointer& provide_center_minorant();


    //@}

  public:
    /// resets all data to initial status of this class, also the bundle parameters
    virtual void clear();

    /// construct a model for the MatrixFunctionOracle pointed to by @a fo
    SOCModel(SOCOracle* fo, CH_Matrix_Classes::Real fun_factor = 1., FunctionTask fun_task = ObjectiveFunction, CBout* cb = 0, int cbinc = -1);

    /// calls clear() and deletes the bundle parameters
    ~SOCModel();


    //----------------------------------------------------------------------
    /** @name implementations of abstract class BundleModel (maybe overloading some of SumBlockModel) */
    //@{


    //eval_function     //as in ConeModel

    //eval_model        //as in ConeModel

    //get_model_aggregate(three parameters) //as in SumBlockModel

    //update_model           //as in ConeModel

    //synchronize_ids       //as in SumBlockModel

    //center_modified        //as in SumBlockModel

    //recompute center      //as in ConeModel

    //model_aggregate_modified      //as in ConeModel

    //provide_model_aggregate  //as in ConeModel

    // apply_modfication    // as in SumBlockModel

    // add_variable_metric  //as in SumBlockModel

    //check_center_validity_by_candidate  //as in ConeModel

    //@}

    //----------------------------------------------------------------------
    /** @name implemented/overloaded routines of SumBlockModel */
    //@{

    /// returns the oracle
    ModifiableOracleObject* get_oracle_object() {
      return oracle;
    }

    //start_augmodel   //as in ConeModel

    // make_model_aggregate         //as in ConeModel

    //get_model_aggregate(... aft=0)   //as in SumBlockModel

    /// see SumBlockModel::lb_function
    CH_Matrix_Classes::Real lb_function(CH_Matrix_Classes::Integer y_id,
      const CH_Matrix_Classes::Matrix& y);

    // get_function_minorant      //as in ConeModel

    // get_center_minorant        //as in ConeModel

    // adjust_multiplier          //as in ConeModel

    // sumbundle_contribution     //as in ConeModel


     // as in see ConeModel::update_model(ModelUpdate,CH_Matrix_Classes::Integer,const CH_Matrix_Classes::Matrix&,CH_Matrix_Classes::Real,CH_Matrix_Classes::Real,CH_Matrix_Classes::Real&,CH_Matrix_Classes::Real&)

   /// see SumBlockModel::get_data
    BundleData* get_data() {
      return &data;
    }

    /// see SumBlockModel::get_data
    const BundleData* get_data() const {
      return &data;
    }

    /// see SumBlockModel::set_data
    int set_data(BundleData* bd) {
      return data.init(bd);
    }

    //@}

    //----------------------------------------------------------------------
    /** @name messages for direct get/set requestss */
    //@{


    /// see SumBlockModel::get_approximate_primal
    const PrimalData* get_approximate_primal() const {
      return data.get_approximate_primal();
    }

    /// see SumBlockModel::get_center_primal
    const PrimalData* get_center_primal() const {
      if (!(const_cast<SOCModel*>(this))->provide_center_minorant().valid()) return 0; return data.get_center_primal();
    }

    /// see SumBlockModel::get_candidate_primal
    const PrimalData* get_candidate_primal() const {
      if (!(const_cast<SOCModel*>(this))->provide_cand_minorant().valid()) return 0; return data.get_candidate_primal();
    }

    /// see SumBlockModel::call_primal_extender
    int call_primal_extender(PrimalExtender& prex) {
      return data.call_primal_extender(prex);
    }

    /// see SumBlockModel::set_bundle_parameters
    int set_bundle_parameters(const BundleParameters& bp);

    /// see SumBlockModel::get_bundle_parameters
    virtual const BundleParameters* get_bundle_parameters() const {
      return model_selection;
    }

    // clear_model()        //as in ConeModel

    // clear_aggregates()   //as in ConeModel

    /// see SumBlockModel::get_ret_code()
    int get_ret_code() const {
      return ret_code;
    }


    //@}

  //---------------------------------------------------------------------- 
  /**@name overloaded routines of BundleQPData */
  //@{

  /// this outputs the qp model data recursively for testing purposes
    std::ostream& output_bundle_data(std::ostream& out) const;

    //@}
    //---------------------------------------------------------------------- 
    /**@name overloaded routines of SumBlockModel for output and statistics on solution times */
    //@{

    //get_QPcoeff_time()      // as in SumBlockModel

    //get_QPsolve_time()      // as in SumBlockModel

    //get_evalaugmodel_time() // as in SumBlockModel

    //get_evalmodel_time()    // as in SumBlockModel

    //get_updatemodel_time()  // as in SumBlockModel

    //get_eval_time()         // as in SumBlockModel

    /// set output and outputlevel of warnings and errors recursively, see CBout
    void set_out(std::ostream* o = 0, int pril = 1) {
      SumBlockModel::set_out(o, pril);
      if (block)
        block->set_cbout(this, -1);
      if (model_selection)
        model_selection->set_cbout(this, 0);
    }

    //set_cbout()  // calls set_out in SumBlockModel


    //@}


  };

  //@}

}

#endif

