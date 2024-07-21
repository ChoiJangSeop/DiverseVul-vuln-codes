static int report_tree(struct dquot *dquot, unsigned int blk, int depth,
		       char *bitmap,
		       int (*process_dquot) (struct dquot *, void *),
		       void *data)
{
	int entries = 0, i;
	dqbuf_t buf = getdqbuf();
	__le32 *ref = (__le32 *) buf;

	if (!buf)
		return 0;

	read_blk(dquot->dq_h, blk, buf);
	if (depth == QT_TREEDEPTH - 1) {
		for (i = 0; i < QT_BLKSIZE >> 2; i++) {
			blk = ext2fs_le32_to_cpu(ref[i]);
			check_reference(dquot->dq_h, blk);
			if (blk && !get_bit(bitmap, blk))
				entries += report_block(dquot, blk, bitmap,
							process_dquot, data);
		}
	} else {
		for (i = 0; i < QT_BLKSIZE >> 2; i++) {
			blk = ext2fs_le32_to_cpu(ref[i]);
			if (blk) {
				check_reference(dquot->dq_h, blk);
				entries += report_tree(dquot, blk, depth + 1,
						       bitmap, process_dquot,
						       data);
			}
		}
	}
	freedqbuf(buf);
	return entries;
}