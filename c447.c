static int do_sync(unsigned int num_qd, struct gfs2_quota_data **qda)
{
	struct gfs2_sbd *sdp = (*qda)->qd_gl->gl_sbd;
	struct gfs2_inode *ip = GFS2_I(sdp->sd_quota_inode);
	unsigned int data_blocks, ind_blocks;
	struct gfs2_holder *ghs, i_gh;
	unsigned int qx, x;
	struct gfs2_quota_data *qd;
	loff_t offset;
	unsigned int nalloc = 0, blocks;
	struct gfs2_alloc *al = NULL;
	int error;

	gfs2_write_calc_reserv(ip, sizeof(struct gfs2_quota),
			      &data_blocks, &ind_blocks);

	ghs = kcalloc(num_qd, sizeof(struct gfs2_holder), GFP_NOFS);
	if (!ghs)
		return -ENOMEM;

	sort(qda, num_qd, sizeof(struct gfs2_quota_data *), sort_qd, NULL);
	mutex_lock_nested(&ip->i_inode.i_mutex, I_MUTEX_QUOTA);
	for (qx = 0; qx < num_qd; qx++) {
		error = gfs2_glock_nq_init(qda[qx]->qd_gl, LM_ST_EXCLUSIVE,
					   GL_NOCACHE, &ghs[qx]);
		if (error)
			goto out;
	}

	error = gfs2_glock_nq_init(ip->i_gl, LM_ST_EXCLUSIVE, 0, &i_gh);
	if (error)
		goto out;

	for (x = 0; x < num_qd; x++) {
		int alloc_required;

		offset = qd2offset(qda[x]);
		error = gfs2_write_alloc_required(ip, offset,
						  sizeof(struct gfs2_quota),
						  &alloc_required);
		if (error)
			goto out_gunlock;
		if (alloc_required)
			nalloc++;
	}

	al = gfs2_alloc_get(ip);
	if (!al) {
		error = -ENOMEM;
		goto out_gunlock;
	}
	/* 
	 * 1 blk for unstuffing inode if stuffed. We add this extra
	 * block to the reservation unconditionally. If the inode
	 * doesn't need unstuffing, the block will be released to the 
	 * rgrp since it won't be allocated during the transaction
	 */
	al->al_requested = 1;
	/* +1 in the end for block requested above for unstuffing */
	blocks = num_qd * data_blocks + RES_DINODE + num_qd + 1;

	if (nalloc)
		al->al_requested += nalloc * (data_blocks + ind_blocks);		
	error = gfs2_inplace_reserve(ip);
	if (error)
		goto out_alloc;

	if (nalloc)
		blocks += al->al_rgd->rd_length + nalloc * ind_blocks + RES_STATFS;

	error = gfs2_trans_begin(sdp, blocks, 0);
	if (error)
		goto out_ipres;

	for (x = 0; x < num_qd; x++) {
		qd = qda[x];
		offset = qd2offset(qd);
		error = gfs2_adjust_quota(ip, offset, qd->qd_change_sync, qd, NULL);
		if (error)
			goto out_end_trans;

		do_qc(qd, -qd->qd_change_sync);
	}

	error = 0;

out_end_trans:
	gfs2_trans_end(sdp);
out_ipres:
	gfs2_inplace_release(ip);
out_alloc:
	gfs2_alloc_put(ip);
out_gunlock:
	gfs2_glock_dq_uninit(&i_gh);
out:
	while (qx--)
		gfs2_glock_dq_uninit(&ghs[qx]);
	mutex_unlock(&ip->i_inode.i_mutex);
	kfree(ghs);
	gfs2_log_flush(ip->i_gl->gl_sbd, ip->i_gl);
	return error;
}