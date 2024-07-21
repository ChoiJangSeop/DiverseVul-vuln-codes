void intel_guc_ads_reset(struct intel_guc *guc)
{
	if (!guc->ads_vma)
		return;
	__guc_ads_init(guc);
}