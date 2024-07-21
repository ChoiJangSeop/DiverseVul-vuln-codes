static int v2_init_io(struct quota_handle *h)
{
	struct v2_disk_dqinfo ddqinfo;

	h->qh_info.u.v2_mdqi.dqi_qtree.dqi_entry_size =
		sizeof(struct v2r1_disk_dqblk);
	h->qh_info.u.v2_mdqi.dqi_qtree.dqi_ops = &v2r1_fmt_ops;

	/* Read information about quotafile */
	if (h->e2fs_read(&h->qh_qf, V2_DQINFOOFF, &ddqinfo,
			 sizeof(ddqinfo)) != sizeof(ddqinfo))
		return -1;
	v2_disk2memdqinfo(&h->qh_info, &ddqinfo);
	return 0;
}