static void __guc_ads_init(struct intel_guc *guc)
{
	struct intel_gt *gt = guc_to_gt(guc);
	struct __guc_ads_blob *blob = guc->ads_blob;
	const u32 skipped_size = LRC_PPHWSP_SZ * PAGE_SIZE + LR_HW_CONTEXT_SIZE;
	u32 base;
	u8 engine_class;

	/* GuC scheduling policies */
	guc_policies_init(&blob->policies);

	/*
	 * GuC expects a per-engine-class context image and size
	 * (minus hwsp and ring context). The context image will be
	 * used to reinitialize engines after a reset. It must exist
	 * and be pinned in the GGTT, so that the address won't change after
	 * we have told GuC where to find it. The context size will be used
	 * to validate that the LRC base + size fall within allowed GGTT.
	 */
	for (engine_class = 0; engine_class <= MAX_ENGINE_CLASS; ++engine_class) {
		if (engine_class == OTHER_CLASS)
			continue;
		/*
		 * TODO: Set context pointer to default state to allow
		 * GuC to re-init guilty contexts after internal reset.
		 */
		blob->ads.golden_context_lrca[engine_class] = 0;
		blob->ads.eng_state_size[engine_class] =
			intel_engine_context_size(guc_to_gt(guc),
						  engine_class) -
			skipped_size;
	}

	/* System info */
	blob->system_info.slice_enabled = hweight8(gt->info.sseu.slice_mask);
	blob->system_info.rcs_enabled = 1;
	blob->system_info.bcs_enabled = 1;

	blob->system_info.vdbox_enable_mask = VDBOX_MASK(gt);
	blob->system_info.vebox_enable_mask = VEBOX_MASK(gt);
	blob->system_info.vdbox_sfc_support_mask = gt->info.vdbox_sfc_access;

	base = intel_guc_ggtt_offset(guc, guc->ads_vma);

	/* Clients info  */
	guc_ct_pool_entries_init(blob->ct_pool, ARRAY_SIZE(blob->ct_pool));

	blob->clients_info.clients_num = 1;
	blob->clients_info.ct_pool_addr = base + ptr_offset(blob, ct_pool);
	blob->clients_info.ct_pool_count = ARRAY_SIZE(blob->ct_pool);

	/* ADS */
	blob->ads.scheduler_policies = base + ptr_offset(blob, policies);
	blob->ads.reg_state_buffer = base + ptr_offset(blob, reg_state_buffer);
	blob->ads.reg_state_addr = base + ptr_offset(blob, reg_state);
	blob->ads.gt_system_info = base + ptr_offset(blob, system_info);
	blob->ads.clients_info = base + ptr_offset(blob, clients_info);

	i915_gem_object_flush_map(guc->ads_vma->obj);
}