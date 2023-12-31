/* ****************************************************************************

    Copyright (C) 2004-2021  Christoph Helmberg

    ConicBundle, Version 1.a.2
    File:  CBsources/CB_CSolver.cxx
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



#include "CB_CSolver.hxx"


using namespace CH_Matrix_Classes;
using namespace ConicBundle;

CB_CSolver::CB_CSolver(bool nb) {
  no_bundle = nb;
  solver = new MatrixCBSolver;
}

CB_CSolver::~CB_CSolver() {
  for (std::map<void*, CFunction*>::iterator i = funmap.begin();
    i != funmap.end();
    ++i) {
    delete i->second;
  }
  funmap.clear();
  delete solver;
}


extern "C" {

  cb_problemp cb_construct_problem(int no_bundle) {
    cb_problemp prob = new CB_CSolver(no_bundle != 0);
    return prob;
  }

  int cb_destruct_problem(cb_problemp* p) {
    delete* p;
    *p = 0;
    return 0;
  }

  void cb_clear(cb_problemp p) {
    assert(p);
    p->solver->clear();
    for (std::map<void*, CFunction*>::iterator i = p->funmap.begin();
      i != p->funmap.end();
      ++i) {
      delete i->second;
    }
    p->funmap.clear();
  }


  void cb_set_defaults(cb_problemp p) {
    assert(p);
    p->solver->set_defaults();
  }

  int cb_init_problem(cb_problemp prob, int m, double* lowerb, double* upperb, double offset) {
    assert(prob);
    cb_clear(prob);
    Matrix lb;
    Matrix* lbp = 0;
    if (lowerb != 0) {
      lbp = &lb;
      lb.init(m, 1, lowerb);
    }
    Matrix ub;
    Matrix* ubp = 0;
    if (upperb != 0) {
      ubp = &ub;
      ub.init(m, 1, upperb);
    }
    return prob->solver->init_problem(m, lbp, ubp, 0, 0, offset);
  }

  int cb_add_function(cb_problemp p, void* function_key, cb_functionp f,
    cb_subgextp se, int primaldim, double fun_factor, int fun_task,
    cb_affinefunctiontransformationp aft) {
    assert(p);
    if (p->funmap.find(function_key) != p->funmap.end()) return 1;
    CFunction* cf = new CFunction(function_key, f, se, primaldim);
    if (cf == 0) return 1;
    p->funmap[function_key] = cf;
    return p->solver->add_function(*cf, fun_factor, (FunctionTask)fun_task, aft, true);
  }

  int cb_set_lower_bound(cb_problemp p, int i, double lb) {
    assert(p);
    return p->solver->set_lower_bound(i, lb);
  }

  int cb_set_upper_bound(cb_problemp p, int i, double ub) {
    assert(p);
    return p->solver->set_upper_bound(i, ub);
  }

  int cb_append_variables(cb_problemp p,
    int n_append, double* lowerb, double* upperb) {
    assert(p);
    Matrix lb;
    Matrix* lbp = 0;
    if (lowerb != 0) {
      lbp = &lb;
      lb.init(n_append, 1, lowerb);
    }
    Matrix ub;
    Matrix* ubp = 0;
    if (upperb != 0) {
      ubp = &ub;
      ub.init(n_append, 1, upperb);
    }
    return p->solver->append_variables(n_append, lbp, ubp, 0);
  }

  int cb_delete_variables(cb_problemp p,
    int n_del, int* delind, int* map_to_old) {
    assert(p);
    assert(n_del >= 0);
    if (n_del == 0) return 0;
    Indexmatrix di(n_del, 1, delind);
    Indexmatrix oi;
    int ret_code = p->solver->delete_variables(di, oi);
    for (int i = 0; i < oi.dim(); i++) {
      map_to_old[i] = oi[i];
    }
    return ret_code;
  }

  int cb_reassign_variables(cb_problemp p,
    int n_assign, int* assignind) {
    assert(p);
    assert(n_assign >= 0);
    Indexmatrix di(n_assign, 1, assignind);
    return p->solver->reassign_variables(di);
  }


  int cb_solve(cb_problemp p, int maxsteps, int stop_at_descent_steps) {
    assert(p);
    if (p->no_bundle) {
      BundleParameters bp(2, 10);
      p->solver->set_sumbundle(true, 0, &bp);
    }
    return p->solver->solve(maxsteps, stop_at_descent_steps != 0);
  }

  int cb_termination_code(cb_problemp p) {
    assert(p);
    return p->solver->termination_code();
  }


  int cb_print_termination_code(cb_problemp p) {
    assert(p);
    p->solver->print_termination_code(std::cout);
    std::cout.flush();
    return 0;
  }


  double cb_get_objval(cb_problemp p) {
    assert(p);
    return p->solver->get_objval();
  }


  int cb_get_center(cb_problemp p, double* cp) {
    assert(p);
    Matrix center;
    int ret_code = p->solver->get_center(center);
    for (int i = 0; i < center.dim(); i++) {
      cp[i] = center[i];
    }
    return ret_code;
  }

  double cb_get_sgnorm(cb_problemp p) {
    assert(p);
    return p->solver->get_sgnorm();
  }


  int cb_get_subgradient(cb_problemp p, double* subgradient) {
    assert(p);
    Matrix subg;
    int ret_code = p->solver->get_subgradient(subg);
    for (int i = 0; i < subg.dim(); i++) {
      subgradient[i] = subg[i];
    }
    return ret_code;
  }


  double cb_get_candidate_value(cb_problemp p) {
    assert(p);
    return p->solver->get_candidate_value();
  }


  int cb_get_candidate(cb_problemp p, double* cp) {
    assert(p);
    Matrix center;
    int ret_code = p->solver->get_candidate(center);
    for (int i = 0; i < center.dim(); i++) {
      cp[i] = center[i];
    }
    return ret_code;
  }

  int cb_set_term_relprec(cb_problemp p, double term_relprec) {
    assert(p);
    return p->solver->set_term_relprec(term_relprec);
  }


  int cb_set_new_center_point(cb_problemp p, double* cp) {
    assert(p);
    int dim = p->solver->get_dim();
    if (dim < 0) return 1;
    Matrix center(dim, 1, cp);
    int ret_code = p->solver->set_new_center_point(center);
    return ret_code;
  }


  int cb_get_function_status(cb_problemp p, void* fk) {
    assert(p);
    assert(p->funmap.find(fk) == p->funmap.end());
    return p->solver->get_function_status(*p->funmap[fk]);
  }

  int cb_get_approximate_slacks(cb_problemp p, double* sl) {
    assert(p);
    Matrix slacks;
    int ret_code = p->solver->get_approximate_slacks(slacks);
    for (int i = 0; i < slacks.dim(); i++) {
      sl[i] = slacks[i];
    }
    return ret_code;
  }



  int cb_get_approximate_primal(cb_problemp p, void* fk, double* primal) {
    assert(p);
    if (p->funmap.find(fk) == p->funmap.end()) return 1;
    const PrimalMatrix* prp = dynamic_cast<const PrimalMatrix*>(p->solver->get_approximate_primal(*p->funmap[fk]));
    if (prp) {
      for (int i = 0; i < prp->dim(); i++) {
        primal[i] = (*prp)[i];
      }
      return 0;
    }
    return 1;
  }

  int cb_get_center_primal(cb_problemp p, void* fk, double* primal) {
    assert(p);
    if (p->funmap.find(fk) == p->funmap.end()) return 1;
    const PrimalMatrix* prp = dynamic_cast<const PrimalMatrix*>(p->solver->get_center_primal(*p->funmap[fk]));
    if (prp) {
      for (int i = 0; i < prp->dim(); i++) {
        primal[i] = (*prp)[i];
      }
      return 0;
    }
    return 1;
  }

  int cb_get_candidate_primal(cb_problemp p, void* fk, double* primal) {
    assert(p);
    if (p->funmap.find(fk) == p->funmap.end()) return 1;
    const PrimalMatrix* prp = dynamic_cast<const PrimalMatrix*>(p->solver->get_candidate_primal(*p->funmap[fk]));
    if (prp) {
      for (int i = 0; i < prp->dim(); i++) {
        primal[i] = (*prp)[i];
      }
      return 0;
    }
    return 1;
  }

  int cb_set_max_modelsize(cb_problemp p, void* fk,
    int modelsize) {
    assert(p);
    if (p->funmap.find(fk) == p->funmap.end()) return 1;
    return p->solver->set_max_modelsize(modelsize, p->funmap[fk]);
  }

  int cb_set_max_bundlesize(cb_problemp p, void* fk,
    int bundlesize) {
    assert(p);
    if (p->funmap.find(fk) == p->funmap.end()) return 1;
    return p->solver->set_max_bundlesize(bundlesize, p->funmap[fk]);
  }

  int cb_set_max_new_subgradients(cb_problemp p, void* fk,
    int max_new_subg) {
    assert(p);
    if (p->funmap.find(fk) == p->funmap.end()) return 1;
    p->funmap[fk]->set_max_new(max_new_subg);
    return 0;
  }

  int cb_get_bundle_parameters(cb_problemp p, void* fk,
    int* modelsize, int* bundlesize) {
    assert(p);
    if (p->funmap.find(fk) == p->funmap.end()) return 1;
    const BundleParameters* bp = dynamic_cast<const BundleParameters*>(p->solver->get_bundle_parameters(p->funmap[fk]));
    if (bp) {
      if (modelsize)
        *modelsize = bp->get_max_model_size();
      if (bundlesize)
        *bundlesize = bp->get_max_bundle_size();
      return 0;
    }
    return 1;
  }


  int cb_reinit_function_model(cb_problemp p, void* fk) {
    assert(p);
    if (p->funmap.find(fk) == p->funmap.end()) return 1;
    return p->solver->reinit_function_model(p->funmap[fk]);
  }


  double cb_get_last_weight(cb_problemp p) {
    assert(p);
    return p->solver->get_last_weight();
  }


  int cb_set_next_weight(cb_problemp p, double weight) {
    assert(p);
    return p->solver->set_next_weight(weight);
  }


  int cb_set_min_weight(cb_problemp p, double min_weight) {
    assert(p);
    return p->solver->set_min_weight(min_weight);
  }


  int cb_set_max_weight(cb_problemp p, double max_weight) {
    assert(p);
    return p->solver->set_max_weight(max_weight);
  }

  int cb_set_variable_metric(cb_problemp p, int do_variable_metric) {
    assert(p);
    return p->solver->set_variable_metric(do_variable_metric);
  }

  void cb_clear_fail_counts(cb_problemp p) {
    assert(p);
    p->solver->clear_fail_counts();
  }

  void cb_set_eval_limit(cb_problemp p, int eval_limit) {
    assert(p);
    p->solver->set_eval_limit(eval_limit);
  }


  void cb_set_inner_update_limit(cb_problemp p, int update_limit) {
    assert(p);
    p->solver->set_inner_update_limit(update_limit);
  }

  int cb_get_dim(cb_problemp p) {
    assert(p);
    return p->solver->get_dim();
  }

  int cb_get_n_functions(cb_problemp p) {
    assert(p);
    return p->solver->get_n_functions();
  }


  double cb_get_minus_infinity() {
    return CB_minus_infinity;
  }

  double cb_get_plus_infinity() {
    return CB_plus_infinity;
  }

  void cb_set_active_bounds_fixing(cb_problemp p, int allow) {
    p->solver->set_active_bounds_fixing(allow != 0);
  }

  int cb_get_fixed_active_bounds(cb_problemp p, int* indicator) {
    assert(p);
    const Indexmatrix* ind = p->solver->get_fixed_active_bounds();
    if (ind) {
      for (int i = 0; i < ind->rowdim(); i++) {
        indicator[i] = (*ind)[i];
      }
    } else {
      for (int i = 0; i < p->solver->get_dim(); i++) {
        indicator[i] = 0;
      }
    }
    return 0;
  }

  void cb_set_print_level(cb_problemp p, int pril) {
    assert(p);
    if (pril < 0) p->solver->set_out(0, 0);
    p->solver->set_out(&std::cout, pril);
  }


}//end extern "C"

