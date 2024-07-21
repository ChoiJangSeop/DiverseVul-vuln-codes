int qtree_scan_dquots(struct quota_handle *h,
		      int (*process_dquot) (struct dquot *, void *),
		      void *data)
{
	char *bitmap;
	struct v2_mem_dqinfo *v2info = &h->qh_info.u.v2_mdqi;
	struct qtree_mem_dqinfo *info = &v2info->dqi_qtree;
	struct dquot *dquot = get_empty_dquot();

	if (!dquot)
		return -1;

	dquot->dq_h = h;
	if (ext2fs_get_memzero((info->dqi_blocks + 7) >> 3, &bitmap)) {
		ext2fs_free_mem(&dquot);
		return -1;
	}
	v2info->dqi_used_entries = report_tree(dquot, QT_TREEOFF, 0, bitmap,
					       process_dquot, data);
	v2info->dqi_data_blocks = find_set_bits(bitmap, info->dqi_blocks);
	ext2fs_free_mem(&bitmap);
	ext2fs_free_mem(&dquot);
	return 0;
}