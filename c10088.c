int intel_guc_ads_create(struct intel_guc *guc)
{
	const u32 size = PAGE_ALIGN(sizeof(struct __guc_ads_blob));
	int ret;

	GEM_BUG_ON(guc->ads_vma);

	ret = intel_guc_allocate_and_map_vma(guc, size, &guc->ads_vma,
					     (void **)&guc->ads_blob);

	if (ret)
		return ret;

	__guc_ads_init(guc);

	return 0;
}