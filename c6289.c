static int _rpc_file_bcast(slurm_msg_t *msg)
{
	int rc;
	int64_t offset, inx;
	file_bcast_info_t *file_info;
	file_bcast_msg_t *req = msg->data;
	file_bcast_info_t key;

	key.uid = g_slurm_auth_get_uid(msg->auth_cred, conf->auth_info);
	key.gid = g_slurm_auth_get_gid(msg->auth_cred, conf->auth_info);
	key.fname = req->fname;

	rc = _valid_sbcast_cred(req, key.uid, req->block_no, &key.job_id);
	if ((rc != SLURM_SUCCESS) && !_slurm_authorized_user(key.uid))
		return rc;

#if 0
	info("last_block=%u force=%u modes=%o",
	     req->last_block, req->force, req->modes);
	info("uid=%u gid=%u atime=%lu mtime=%lu block_len[0]=%u",
	     req->uid, req->gid, req->atime, req->mtime, req->block_len);
#if 0
	/* when the file being transferred is binary, the following line
	 * can break the terminal output for slurmd */
	info("req->block[0]=%s, @ %lu", \
	     req->block[0], (unsigned long) &req->block);
#endif
#endif

	if (req->block_no == 1) {
		info("sbcast req_uid=%u job_id=%u fname=%s block_no=%u",
		     key.uid, key.job_id, key.fname, req->block_no);
	} else {
		debug("sbcast req_uid=%u job_id=%u fname=%s block_no=%u",
		      key.uid, key.job_id, key.fname, req->block_no);
	}

	/* first block must register the file and open fd/mmap */
	if (req->block_no == 1) {
		if ((rc = _file_bcast_register_file(msg, &key)))
			return rc;
	}

	_fb_rdlock();
	if (!(file_info = _bcast_lookup_file(&key))) {
		error("No registered file transfer for uid %u file `%s`.",
		      key.uid, key.fname);
		_fb_rdunlock();
		return SLURM_ERROR;
	}

	/* now decompress file */
	if (bcast_decompress_data(req) < 0) {
		error("sbcast: data decompression error for UID %u, file %s",
		      key.uid, key.fname);
		_fb_rdunlock();
		return SLURM_FAILURE;
	}

	offset = 0;
	while (req->block_len - offset) {
		inx = write(file_info->fd, &req->block[offset],
			    (req->block_len - offset));
		if (inx == -1) {
			if ((errno == EINTR) || (errno == EAGAIN))
				continue;
			error("sbcast: uid:%u can't write `%s`: %m",
			      key.uid, key.fname);
			_fb_rdunlock();
			return SLURM_FAILURE;
		}
		offset += inx;
	}

	file_info->last_update = time(NULL);

	if (req->last_block && fchmod(file_info->fd, (req->modes & 0777))) {
		error("sbcast: uid:%u can't chmod `%s`: %m",
		      key.uid, key.fname);
	}
	if (req->last_block && fchown(file_info->fd, key.uid, key.gid)) {
		error("sbcast: uid:%u gid:%u can't chown `%s`: %m",
		      key.uid, key.gid, key.fname);
	}
	if (req->last_block && req->atime) {
		struct utimbuf time_buf;
		time_buf.actime  = req->atime;
		time_buf.modtime = req->mtime;
		if (utime(key.fname, &time_buf)) {
			error("sbcast: uid:%u can't utime `%s`: %m",
			      key.uid, key.fname);
		}
	}

	_fb_rdunlock();

	if (req->last_block) {
		_file_bcast_close_file(&key);
	}
	return SLURM_SUCCESS;
}