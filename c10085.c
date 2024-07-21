static u32 guc_ctl_ctxinfo_flags(struct intel_guc *guc)
{
	u32 flags = 0;

	if (intel_guc_submission_is_used(guc)) {
		u32 ctxnum, base;

		base = intel_guc_ggtt_offset(guc, guc->stage_desc_pool);
		ctxnum = GUC_MAX_STAGE_DESCRIPTORS / 16;

		base >>= PAGE_SHIFT;
		flags |= (base << GUC_CTL_BASE_ADDR_SHIFT) |
			(ctxnum << GUC_CTL_CTXNUM_IN16_SHIFT);
	}
	return flags;
}