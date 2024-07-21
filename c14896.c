smb2_ioctl_query_info(const unsigned int xid,
		      struct cifs_tcon *tcon,
		      struct cifs_sb_info *cifs_sb,
		      __le16 *path, int is_dir,
		      unsigned long p)
{
	struct iqi_vars *vars;
	struct smb_rqst *rqst;
	struct kvec *rsp_iov;
	struct cifs_ses *ses = tcon->ses;
	struct TCP_Server_Info *server = cifs_pick_channel(ses);
	char __user *arg = (char __user *)p;
	struct smb_query_info qi;
	struct smb_query_info __user *pqi;
	int rc = 0;
	int flags = CIFS_CP_CREATE_CLOSE_OP;
	struct smb2_query_info_rsp *qi_rsp = NULL;
	struct smb2_ioctl_rsp *io_rsp = NULL;
	void *buffer = NULL;
	int resp_buftype[3];
	struct cifs_open_parms oparms;
	u8 oplock = SMB2_OPLOCK_LEVEL_NONE;
	struct cifs_fid fid;
	unsigned int size[2];
	void *data[2];
	int create_options = is_dir ? CREATE_NOT_FILE : CREATE_NOT_DIR;

	vars = kzalloc(sizeof(*vars), GFP_ATOMIC);
	if (vars == NULL)
		return -ENOMEM;
	rqst = &vars->rqst[0];
	rsp_iov = &vars->rsp_iov[0];

	resp_buftype[0] = resp_buftype[1] = resp_buftype[2] = CIFS_NO_BUFFER;

	if (copy_from_user(&qi, arg, sizeof(struct smb_query_info)))
		goto e_fault;

	if (qi.output_buffer_length > 1024) {
		kfree(vars);
		return -EINVAL;
	}

	if (!ses || !server) {
		kfree(vars);
		return -EIO;
	}

	if (smb3_encryption_required(tcon))
		flags |= CIFS_TRANSFORM_REQ;

	if (qi.output_buffer_length) {
		buffer = memdup_user(arg + sizeof(struct smb_query_info), qi.output_buffer_length);
		if (IS_ERR(buffer)) {
			kfree(vars);
			return PTR_ERR(buffer);
		}
	}

	/* Open */
	rqst[0].rq_iov = &vars->open_iov[0];
	rqst[0].rq_nvec = SMB2_CREATE_IOV_SIZE;

	memset(&oparms, 0, sizeof(oparms));
	oparms.tcon = tcon;
	oparms.disposition = FILE_OPEN;
	oparms.create_options = cifs_create_options(cifs_sb, create_options);
	oparms.fid = &fid;
	oparms.reconnect = false;

	if (qi.flags & PASSTHRU_FSCTL) {
		switch (qi.info_type & FSCTL_DEVICE_ACCESS_MASK) {
		case FSCTL_DEVICE_ACCESS_FILE_READ_WRITE_ACCESS:
			oparms.desired_access = FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE;
			break;
		case FSCTL_DEVICE_ACCESS_FILE_ANY_ACCESS:
			oparms.desired_access = GENERIC_ALL;
			break;
		case FSCTL_DEVICE_ACCESS_FILE_READ_ACCESS:
			oparms.desired_access = GENERIC_READ;
			break;
		case FSCTL_DEVICE_ACCESS_FILE_WRITE_ACCESS:
			oparms.desired_access = GENERIC_WRITE;
			break;
		}
	} else if (qi.flags & PASSTHRU_SET_INFO) {
		oparms.desired_access = GENERIC_WRITE;
	} else {
		oparms.desired_access = FILE_READ_ATTRIBUTES | READ_CONTROL;
	}

	rc = SMB2_open_init(tcon, server,
			    &rqst[0], &oplock, &oparms, path);
	if (rc)
		goto iqinf_exit;
	smb2_set_next_command(tcon, &rqst[0]);

	/* Query */
	if (qi.flags & PASSTHRU_FSCTL) {
		/* Can eventually relax perm check since server enforces too */
		if (!capable(CAP_SYS_ADMIN))
			rc = -EPERM;
		else  {
			rqst[1].rq_iov = &vars->io_iov[0];
			rqst[1].rq_nvec = SMB2_IOCTL_IOV_SIZE;

			rc = SMB2_ioctl_init(tcon, server,
					     &rqst[1],
					     COMPOUND_FID, COMPOUND_FID,
					     qi.info_type, true, buffer,
					     qi.output_buffer_length,
					     CIFSMaxBufSize -
					     MAX_SMB2_CREATE_RESPONSE_SIZE -
					     MAX_SMB2_CLOSE_RESPONSE_SIZE);
		}
	} else if (qi.flags == PASSTHRU_SET_INFO) {
		/* Can eventually relax perm check since server enforces too */
		if (!capable(CAP_SYS_ADMIN))
			rc = -EPERM;
		else if (qi.output_buffer_length < 8)
			rc = -EINVAL;
		else {
			rqst[1].rq_iov = &vars->si_iov[0];
			rqst[1].rq_nvec = 1;

			/* MS-FSCC 2.4.13 FileEndOfFileInformation */
			size[0] = 8;
			data[0] = buffer;

			rc = SMB2_set_info_init(tcon, server,
					&rqst[1],
					COMPOUND_FID, COMPOUND_FID,
					current->tgid,
					FILE_END_OF_FILE_INFORMATION,
					SMB2_O_INFO_FILE, 0, data, size);
		}
	} else if (qi.flags == PASSTHRU_QUERY_INFO) {
		rqst[1].rq_iov = &vars->qi_iov[0];
		rqst[1].rq_nvec = 1;

		rc = SMB2_query_info_init(tcon, server,
				  &rqst[1], COMPOUND_FID,
				  COMPOUND_FID, qi.file_info_class,
				  qi.info_type, qi.additional_information,
				  qi.input_buffer_length,
				  qi.output_buffer_length, buffer);
	} else { /* unknown flags */
		cifs_tcon_dbg(VFS, "Invalid passthru query flags: 0x%x\n",
			      qi.flags);
		rc = -EINVAL;
	}

	if (rc)
		goto iqinf_exit;
	smb2_set_next_command(tcon, &rqst[1]);
	smb2_set_related(&rqst[1]);

	/* Close */
	rqst[2].rq_iov = &vars->close_iov[0];
	rqst[2].rq_nvec = 1;

	rc = SMB2_close_init(tcon, server,
			     &rqst[2], COMPOUND_FID, COMPOUND_FID, false);
	if (rc)
		goto iqinf_exit;
	smb2_set_related(&rqst[2]);

	rc = compound_send_recv(xid, ses, server,
				flags, 3, rqst,
				resp_buftype, rsp_iov);
	if (rc)
		goto iqinf_exit;

	/* No need to bump num_remote_opens since handle immediately closed */
	if (qi.flags & PASSTHRU_FSCTL) {
		pqi = (struct smb_query_info __user *)arg;
		io_rsp = (struct smb2_ioctl_rsp *)rsp_iov[1].iov_base;
		if (le32_to_cpu(io_rsp->OutputCount) < qi.input_buffer_length)
			qi.input_buffer_length = le32_to_cpu(io_rsp->OutputCount);
		if (qi.input_buffer_length > 0 &&
		    le32_to_cpu(io_rsp->OutputOffset) + qi.input_buffer_length
		    > rsp_iov[1].iov_len)
			goto e_fault;

		if (copy_to_user(&pqi->input_buffer_length,
				 &qi.input_buffer_length,
				 sizeof(qi.input_buffer_length)))
			goto e_fault;

		if (copy_to_user((void __user *)pqi + sizeof(struct smb_query_info),
				 (const void *)io_rsp + le32_to_cpu(io_rsp->OutputOffset),
				 qi.input_buffer_length))
			goto e_fault;
	} else {
		pqi = (struct smb_query_info __user *)arg;
		qi_rsp = (struct smb2_query_info_rsp *)rsp_iov[1].iov_base;
		if (le32_to_cpu(qi_rsp->OutputBufferLength) < qi.input_buffer_length)
			qi.input_buffer_length = le32_to_cpu(qi_rsp->OutputBufferLength);
		if (copy_to_user(&pqi->input_buffer_length,
				 &qi.input_buffer_length,
				 sizeof(qi.input_buffer_length)))
			goto e_fault;

		if (copy_to_user(pqi + 1, qi_rsp->Buffer,
				 qi.input_buffer_length))
			goto e_fault;
	}

 iqinf_exit:
	cifs_small_buf_release(rqst[0].rq_iov[0].iov_base);
	cifs_small_buf_release(rqst[1].rq_iov[0].iov_base);
	cifs_small_buf_release(rqst[2].rq_iov[0].iov_base);
	free_rsp_buf(resp_buftype[0], rsp_iov[0].iov_base);
	free_rsp_buf(resp_buftype[1], rsp_iov[1].iov_base);
	free_rsp_buf(resp_buftype[2], rsp_iov[2].iov_base);
	kfree(vars);
	kfree(buffer);
	return rc;

e_fault:
	rc = -EFAULT;
	goto iqinf_exit;
}