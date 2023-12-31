#include "cb_cinterface.h"
#include "matrix.hxx"
#include "indexmat.hxx"
#include "sparsmat.hxx"
#include "symmat.hxx"
#include "sparssym.hxx"
#include "CMgramdense.hxx"
#include "CMgramsparse.hxx"
#include "CMgramsparse_withoutdiag.hxx"
#include "CMlowrankdd.hxx"
#include "CMlowranksd.hxx"
#include "CMlowrankss.hxx"
#include "CMsingleton.hxx"
#include "CMsymdense.hxx"
#include "CMsymsparse.hxx"
#include "SparseCoeffmatMatrix.hxx"
#include "gb_rand.hxx"
#include "Coeffmat.hxx"
#include "MatrixCBSolver.hxx"
#include "PSCPrimal.hxx"
#include "AFTModification.hxx"
#include "GroundsetModification.hxx"
#include "NNCBoxSupportModification.hxx"
#include "PSCAffineModification.hxx"
#include "SOCSupportModification.hxx"
#include "AffineFunctionTransformation.hxx"
#include "MinorantPointer.hxx"
#include "MinorantUseData.hxx"
#include "CBSolver.hxx"
#include "BoxOracle.hxx"
#include "PSCOracle.hxx"
#include "SOCOracle.hxx"
#include "CFunction.hxx"
#include "NNCBoxSupportFunction.hxx"
#include "PSCAffineFunction.hxx"
#include "SOCSupportFunction.hxx"
#include "BoxModelParameters.hxx"
#include "NNCModelParameters.hxx"
#include "PSCModelParameters.hxx"
#include "SOCModelParameters.hxx"
#include "SumBundleParameters.hxx"
#include "AFTData.hxx"
#include "BoxData.hxx"
#include "NNCData.hxx"
#include "PSCData.hxx"
#include "SOCData.hxx"
#include "BundleHKWeight.hxx"
#include "BundleRQBWeight.hxx"
#include "BundleDenseTrustRegionProx.hxx"
#include "BundleDiagonalTrustRegionProx.hxx"
#include "BundleDLRTrustRegionProx.hxx"
#include "BundleIdProx.hxx"
#include "BundleLowRankTrustRegionProx.hxx"
#include "QPSolverParameters.hxx"
#include "QPSolver.hxx"
#include "UQPSolver.hxx"
#include "LPGroundset.hxx"
#include "UnconstrainedGroundset.hxx"
#include "AFTModel.hxx"
#include "BoxModel.hxx"
#include "NNCModel.hxx"
#include "PSCModel.hxx"
#include "SOCModel.hxx"
#include "SumModel.hxx"
#include "PSCVariableMetricSelection.hxx"
#include "VariableMetricSVDSelection.hxx"
#include "QPDirectKKTSolver.hxx"
#include "QPIterativeKKTHAeqSolver.hxx"
#include "QPIterativeKKTHASolver.hxx"
#include "QPKKTSolverComparison.hxx"
#include "SumBundleHandler.hxx"
#include "QPConeModelBlock.hxx"
#include "QPSumModelBlock.hxx"
#include "UQPConeModelBlock.hxx"
#include "UQPSumModelBlock.hxx"
#include "minres.hxx"
#include "pcg.hxx"
#include "psqmr.hxx"
#include "QPKKTSubspaceHPrecond.hxx"
#include "SumBundle.hxx"
#include "BundleSolver.hxx"
#include "BundleTerminator.hxx"
#include "clock.hxx"
using namespace CH_Matrix_Classes;
using namespace ConicBundle;
using namespace CH_Tools;
#define ModelUpdate BundleModel::ModelUpdate

extern "C" {

#include "cb_matrix.cpp"
#include "cb_indexmatrix.cpp"
#include "cb_sparsemat.cpp"
#include "cb_symmatrix.cpp"
#include "cb_sparsesym.cpp"
#include "cb_cmgramdense.cpp"
#include "cb_cmgramsparse.cpp"
#include "cb_cmgramsparse_withoutdiag.cpp"
#include "cb_cmlowrankdd.cpp"
#include "cb_cmlowranksd.cpp"
#include "cb_cmlowrankss.cpp"
#include "cb_cmsingleton.cpp"
#include "cb_cmsymdense.cpp"
#include "cb_cmsymsparse.cpp"
#include "cb_sparsecoeffmatmatrix.cpp"
#include "cb_gb_rand.cpp"
#include "cb_coeffmatinfo.cpp"
#include "cb_primalmatrix.cpp"
#include "cb_matrixcbsolver.cpp"
#include "cb_blockpscprimal.cpp"
#include "cb_densepscprimal.cpp"
#include "cb_gramsparsepscprimal.cpp"
#include "cb_sparsepscprimal.cpp"
#include "cb_aftmodification.cpp"
#include "cb_groundsetmodification.cpp"
#include "cb_nncboxsupportmodification.cpp"
#include "cb_pscaffinemodification.cpp"
#include "cb_socsupportmodification.cpp"
#include "cb_affinefunctiontransformation.cpp"
#include "cb_minorantpointer.cpp"
#include "cb_minorantusedata.cpp"
#include "cb_minorant.cpp"
#include "cb_boxprimalextender.cpp"
#include "cb_boxoracle.cpp"
#include "cb_pscprimalextender.cpp"
#include "cb_pscbundleparameters.cpp"
#include "cb_socprimalextender.cpp"
#include "cb_socbundleparameters.cpp"
#include "cb_cfunctionminorantextender.cpp"
#include "cb_cfunction.cpp"
#include "cb_nncboxsupportminorantextender.cpp"
#include "cb_nncboxsupportfunction.cpp"
#include "cb_pscaffineminorantextender.cpp"
#include "cb_pscaffinefunction.cpp"
#include "cb_socsupportminorantextender.cpp"
#include "cb_socsupportfunction.cpp"
#include "cb_boxmodelparameters.cpp"
#include "cb_nncmodelparameters.cpp"
#include "cb_pscmodelparameters.cpp"
#include "cb_socmodelparameters.cpp"
#include "cb_sumbundleparameters.cpp"
#include "cb_aftdata.cpp"
#include "cb_boxdata.cpp"
#include "cb_nncdata.cpp"
#include "cb_pscdata.cpp"
#include "cb_socdata.cpp"
#include "cb_bundlehkweight.cpp"
#include "cb_bundlerqbweight.cpp"
#include "cb_bundledensetrustregionprox.cpp"
#include "cb_bundlediagonaltrustregionprox.cpp"
#include "cb_bundledlrtrustregionprox.cpp"
#include "cb_bundleidprox.cpp"
#include "cb_bundlelowranktrustregionprox.cpp"
#include "cb_qpsolverparameters.cpp"
#include "cb_qpsolver.cpp"
#include "cb_uqpsolver.cpp"
#include "cb_lpgroundset.cpp"
#include "cb_unconstrainedgroundset.cpp"
#include "cb_aftmodel.cpp"
#include "cb_boxmodel.cpp"
#include "cb_nncmodel.cpp"
#include "cb_pscmodel.cpp"
#include "cb_socmodel.cpp"
#include "cb_summodel.cpp"
#include "cb_pscvariablemetricselection.cpp"
#include "cb_variablemetricsvdselection.cpp"
#include "cb_qpdirectkktsolver.cpp"
#include "cb_qpiterativekkthaeqsolver.cpp"
#include "cb_qpiterativekkthasolver.cpp"
#include "cb_qpkktsolvercomparison.cpp"
#include "cb_sumbundlehandler.cpp"
#include "cb_qpconemodelblock.cpp"
#include "cb_qpsummodelblock.cpp"
#include "cb_uqpconemodelblock.cpp"
#include "cb_uqpsummodelblock.cpp"
#include "cb_minres.cpp"
#include "cb_pcg.cpp"
#include "cb_psqmr.cpp"
#include "cb_qpkktsubspacehprecond.cpp"
#include "cb_sumbundle.cpp"
#include "cb_bundlesolver.cpp"
#include "cb_bundleterminator.cpp"
#include "cb_clock.cpp"
#include "cb_microseconds.cpp"

}