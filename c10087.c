static void guc_init_params(struct intel_guc *guc)
{
	u32 *params = guc->params;
	int i;

	BUILD_BUG_ON(sizeof(guc->params) != GUC_CTL_MAX_DWORDS * sizeof(u32));

	params[GUC_CTL_CTXINFO] = guc_ctl_ctxinfo_flags(guc);
	params[GUC_CTL_LOG_PARAMS] = guc_ctl_log_params_flags(guc);
	params[GUC_CTL_FEATURE] = guc_ctl_feature_flags(guc);
	params[GUC_CTL_DEBUG] = guc_ctl_debug_flags(guc);
	params[GUC_CTL_ADS] = guc_ctl_ads_flags(guc);

	for (i = 0; i < GUC_CTL_MAX_DWORDS; i++)
		DRM_DEBUG_DRIVER("param[%2d] = %#x\n", i, params[i]);
}