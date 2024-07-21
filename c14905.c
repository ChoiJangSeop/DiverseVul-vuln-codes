RZ_API void rz_analysis_extract_vars(RzAnalysis *analysis, RzAnalysisFunction *fcn, RzAnalysisOp *op) {
	rz_return_if_fail(analysis && fcn && op);

	const char *BP = analysis->reg->name[RZ_REG_NAME_BP];
	const char *SP = analysis->reg->name[RZ_REG_NAME_SP];
	if (BP) {
		extract_arg(analysis, fcn, op, BP, "+", RZ_ANALYSIS_VAR_KIND_BPV);
		extract_arg(analysis, fcn, op, BP, "-", RZ_ANALYSIS_VAR_KIND_BPV);
	}
	extract_arg(analysis, fcn, op, SP, "+", RZ_ANALYSIS_VAR_KIND_SPV);
}