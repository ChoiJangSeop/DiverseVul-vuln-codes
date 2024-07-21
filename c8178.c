static int report_block(struct dquot *dquot, unsigned int blk, char *bitmap,
			int (*process_dquot) (struct dquot *, void *),
			void *data)
{
	struct qtree_mem_dqinfo *info =
			&dquot->dq_h->qh_info.u.v2_mdqi.dqi_qtree;
	dqbuf_t buf = getdqbuf();
	struct qt_disk_dqdbheader *dh;
	char *ddata;
	int entries, i;

	if (!buf)
		return 0;

	set_bit(bitmap, blk);
	read_blk(dquot->dq_h, blk, buf);
	dh = (struct qt_disk_dqdbheader *)buf;
	ddata = buf + sizeof(struct qt_disk_dqdbheader);
	entries = ext2fs_le16_to_cpu(dh->dqdh_entries);
	for (i = 0; i < qtree_dqstr_in_blk(info);
			i++, ddata += info->dqi_entry_size)
		if (!qtree_entry_unused(info, ddata)) {
			dquot->dq_dqb.u.v2_mdqb.dqb_off =
				(blk << QT_BLKSIZE_BITS) +
				sizeof(struct qt_disk_dqdbheader) +
				i * info->dqi_entry_size;
			info->dqi_ops->disk2mem_dqblk(dquot, ddata);
			if (process_dquot(dquot, data) < 0)
				break;
		}
	freedqbuf(buf);
	return entries;
}