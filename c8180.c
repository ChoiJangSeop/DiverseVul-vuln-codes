static void check_reference(struct quota_handle *h, unsigned int blk)
{
	if (blk >= h->qh_info.u.v2_mdqi.dqi_qtree.dqi_blocks)
		log_err("Illegal reference (%u >= %u) in %s quota file. "
			"Quota file is probably corrupted.\n"
			"Please run e2fsck (8) to fix it.",
			blk,
			h->qh_info.u.v2_mdqi.dqi_qtree.dqi_blocks,
			quota_type2name(h->qh_type));
}