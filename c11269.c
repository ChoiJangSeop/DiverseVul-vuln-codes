static loff_t find_tree_dqentry(struct qtree_mem_dqinfo *info,
				struct dquot *dquot, uint blk, int depth)
{
	char *buf = kmalloc(info->dqi_usable_bs, GFP_NOFS);
	loff_t ret = 0;
	__le32 *ref = (__le32 *)buf;

	if (!buf)
		return -ENOMEM;
	ret = read_blk(info, blk, buf);
	if (ret < 0) {
		quota_error(dquot->dq_sb, "Can't read quota tree block %u",
			    blk);
		goto out_buf;
	}
	ret = 0;
	blk = le32_to_cpu(ref[get_index(info, dquot->dq_id, depth)]);
	if (!blk)	/* No reference? */
		goto out_buf;
	if (depth < info->dqi_qtree_depth - 1)
		ret = find_tree_dqentry(info, dquot, blk, depth+1);
	else
		ret = find_block_dqentry(info, dquot, blk);
out_buf:
	kfree(buf);
	return ret;
}