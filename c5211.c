static NTSTATUS fd_open_atomic(struct connection_struct *conn,
			files_struct *fsp,
			int flags,
			mode_t mode,
			bool *file_created)
{
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	bool file_existed = VALID_STAT(fsp->fsp_name->st);

	*file_created = false;

	if (!(flags & O_CREAT)) {
		/*
		 * We're not creating the file, just pass through.
		 */
		return fd_open(conn, fsp, flags, mode);
	}

	if (flags & O_EXCL) {
		/*
		 * Fail if already exists, just pass through.
		 */
		status = fd_open(conn, fsp, flags, mode);

		/*
		 * Here we've opened with O_CREAT|O_EXCL. If that went
		 * NT_STATUS_OK, we *know* we created this file.
		 */
		*file_created = NT_STATUS_IS_OK(status);

		return status;
	}

	/*
	 * Now it gets tricky. We have O_CREAT, but not O_EXCL.
	 * To know absolutely if we created the file or not,
	 * we can never call O_CREAT without O_EXCL. So if
	 * we think the file existed, try without O_CREAT|O_EXCL.
	 * If we think the file didn't exist, try with
	 * O_CREAT|O_EXCL. Keep bouncing between these two
	 * requests until either the file is created, or
	 * opened. Either way, we keep going until we get
	 * a returnable result (error, or open/create).
	 */

	while(1) {
		int curr_flags = flags;

		if (file_existed) {
			/* Just try open, do not create. */
			curr_flags &= ~(O_CREAT);
			status = fd_open(conn, fsp, curr_flags, mode);
			if (NT_STATUS_EQUAL(status,
					NT_STATUS_OBJECT_NAME_NOT_FOUND)) {
				/*
				 * Someone deleted it in the meantime.
				 * Retry with O_EXCL.
				 */
				file_existed = false;
				DEBUG(10,("fd_open_atomic: file %s existed. "
					"Retry.\n",
					smb_fname_str_dbg(fsp->fsp_name)));
					continue;
			}
		} else {
			/* Try create exclusively, fail if it exists. */
			curr_flags |= O_EXCL;
			status = fd_open(conn, fsp, curr_flags, mode);
			if (NT_STATUS_EQUAL(status,
					NT_STATUS_OBJECT_NAME_COLLISION)) {
				/*
				 * Someone created it in the meantime.
				 * Retry without O_CREAT.
				 */
				file_existed = true;
				DEBUG(10,("fd_open_atomic: file %s "
					"did not exist. Retry.\n",
					smb_fname_str_dbg(fsp->fsp_name)));
				continue;
			}
			if (NT_STATUS_IS_OK(status)) {
				/*
				 * Here we've opened with O_CREAT|O_EXCL
				 * and got success. We *know* we created
				 * this file.
				 */
				*file_created = true;
			}
		}
		/* Create is done, or failed. */
		break;
	}
	return status;
}