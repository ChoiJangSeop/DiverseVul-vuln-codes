static NTSTATUS create_file_unixpath(connection_struct *conn,
				     struct smb_request *req,
				     struct smb_filename *smb_fname,
				     uint32_t access_mask,
				     uint32_t share_access,
				     uint32_t create_disposition,
				     uint32_t create_options,
				     uint32_t file_attributes,
				     uint32_t oplock_request,
				     uint64_t allocation_size,
				     uint32_t private_flags,
				     struct security_descriptor *sd,
				     struct ea_list *ea_list,

				     files_struct **result,
				     int *pinfo)
{
	int info = FILE_WAS_OPENED;
	files_struct *base_fsp = NULL;
	files_struct *fsp = NULL;
	NTSTATUS status;

	DEBUG(10,("create_file_unixpath: access_mask = 0x%x "
		  "file_attributes = 0x%x, share_access = 0x%x, "
		  "create_disposition = 0x%x create_options = 0x%x "
		  "oplock_request = 0x%x private_flags = 0x%x "
		  "ea_list = 0x%p, sd = 0x%p, "
		  "fname = %s\n",
		  (unsigned int)access_mask,
		  (unsigned int)file_attributes,
		  (unsigned int)share_access,
		  (unsigned int)create_disposition,
		  (unsigned int)create_options,
		  (unsigned int)oplock_request,
		  (unsigned int)private_flags,
		  ea_list, sd, smb_fname_str_dbg(smb_fname)));

	if (create_options & FILE_OPEN_BY_FILE_ID) {
		status = NT_STATUS_NOT_SUPPORTED;
		goto fail;
	}

	if (create_options & NTCREATEX_OPTIONS_INVALID_PARAM_MASK) {
		status = NT_STATUS_INVALID_PARAMETER;
		goto fail;
	}

	if (req == NULL) {
		oplock_request |= INTERNAL_OPEN_ONLY;
	}

	if ((conn->fs_capabilities & FILE_NAMED_STREAMS)
	    && (access_mask & DELETE_ACCESS)
	    && !is_ntfs_stream_smb_fname(smb_fname)) {
		/*
		 * We can't open a file with DELETE access if any of the
		 * streams is open without FILE_SHARE_DELETE
		 */
		status = open_streams_for_delete(conn, smb_fname->base_name);

		if (!NT_STATUS_IS_OK(status)) {
			goto fail;
		}
	}

	if ((access_mask & SEC_FLAG_SYSTEM_SECURITY) &&
			!security_token_has_privilege(get_current_nttok(conn),
					SEC_PRIV_SECURITY)) {
		DEBUG(10, ("create_file_unixpath: open on %s "
			"failed - SEC_FLAG_SYSTEM_SECURITY denied.\n",
			smb_fname_str_dbg(smb_fname)));
		status = NT_STATUS_PRIVILEGE_NOT_HELD;
		goto fail;
	}

	if ((conn->fs_capabilities & FILE_NAMED_STREAMS)
	    && is_ntfs_stream_smb_fname(smb_fname)
	    && (!(private_flags & NTCREATEX_OPTIONS_PRIVATE_STREAM_DELETE))) {
		uint32 base_create_disposition;
		struct smb_filename *smb_fname_base = NULL;

		if (create_options & FILE_DIRECTORY_FILE) {
			status = NT_STATUS_NOT_A_DIRECTORY;
			goto fail;
		}

		switch (create_disposition) {
		case FILE_OPEN:
			base_create_disposition = FILE_OPEN;
			break;
		default:
			base_create_disposition = FILE_OPEN_IF;
			break;
		}

		/* Create an smb_filename with stream_name == NULL. */
		smb_fname_base = synthetic_smb_fname(talloc_tos(),
						     smb_fname->base_name,
						     NULL, NULL);
		if (smb_fname_base == NULL) {
			status = NT_STATUS_NO_MEMORY;
			goto fail;
		}

		if (SMB_VFS_STAT(conn, smb_fname_base) == -1) {
			DEBUG(10, ("Unable to stat stream: %s\n",
				   smb_fname_str_dbg(smb_fname_base)));
		}

		/* Open the base file. */
		status = create_file_unixpath(conn, NULL, smb_fname_base, 0,
					      FILE_SHARE_READ
					      | FILE_SHARE_WRITE
					      | FILE_SHARE_DELETE,
					      base_create_disposition,
					      0, 0, 0, 0, 0, NULL, NULL,
					      &base_fsp, NULL);
		TALLOC_FREE(smb_fname_base);

		if (!NT_STATUS_IS_OK(status)) {
			DEBUG(10, ("create_file_unixpath for base %s failed: "
				   "%s\n", smb_fname->base_name,
				   nt_errstr(status)));
			goto fail;
		}
		/* we don't need to low level fd */
		fd_close(base_fsp);
	}

	/*
	 * If it's a request for a directory open, deal with it separately.
	 */

	if (create_options & FILE_DIRECTORY_FILE) {

		if (create_options & FILE_NON_DIRECTORY_FILE) {
			status = NT_STATUS_INVALID_PARAMETER;
			goto fail;
		}

		/* Can't open a temp directory. IFS kit test. */
		if (!(file_attributes & FILE_FLAG_POSIX_SEMANTICS) &&
		     (file_attributes & FILE_ATTRIBUTE_TEMPORARY)) {
			status = NT_STATUS_INVALID_PARAMETER;
			goto fail;
		}

		/*
		 * We will get a create directory here if the Win32
		 * app specified a security descriptor in the
		 * CreateDirectory() call.
		 */

		oplock_request = 0;
		status = open_directory(
			conn, req, smb_fname, access_mask, share_access,
			create_disposition, create_options, file_attributes,
			&info, &fsp);
	} else {

		/*
		 * Ordinary file case.
		 */

		status = file_new(req, conn, &fsp);
		if(!NT_STATUS_IS_OK(status)) {
			goto fail;
		}

		status = fsp_set_smb_fname(fsp, smb_fname);
		if (!NT_STATUS_IS_OK(status)) {
			goto fail;
		}

		if (base_fsp) {
			/*
			 * We're opening the stream element of a
			 * base_fsp we already opened. Set up the
			 * base_fsp pointer.
			 */
			fsp->base_fsp = base_fsp;
		}

		if (allocation_size) {
			fsp->initial_allocation_size = smb_roundup(fsp->conn,
							allocation_size);
		}

		status = open_file_ntcreate(conn,
					    req,
					    access_mask,
					    share_access,
					    create_disposition,
					    create_options,
					    file_attributes,
					    oplock_request,
					    private_flags,
					    &info,
					    fsp);

		if(!NT_STATUS_IS_OK(status)) {
			file_free(req, fsp);
			fsp = NULL;
		}

		if (NT_STATUS_EQUAL(status, NT_STATUS_FILE_IS_A_DIRECTORY)) {

			/* A stream open never opens a directory */

			if (base_fsp) {
				status = NT_STATUS_FILE_IS_A_DIRECTORY;
				goto fail;
			}

			/*
			 * Fail the open if it was explicitly a non-directory
			 * file.
			 */

			if (create_options & FILE_NON_DIRECTORY_FILE) {
				status = NT_STATUS_FILE_IS_A_DIRECTORY;
				goto fail;
			}

			oplock_request = 0;
			status = open_directory(
				conn, req, smb_fname, access_mask,
				share_access, create_disposition,
				create_options,	file_attributes,
				&info, &fsp);
		}
	}

	if (!NT_STATUS_IS_OK(status)) {
		goto fail;
	}

	fsp->base_fsp = base_fsp;

	if ((ea_list != NULL) &&
	    ((info == FILE_WAS_CREATED) || (info == FILE_WAS_OVERWRITTEN))) {
		status = set_ea(conn, fsp, fsp->fsp_name, ea_list);
		if (!NT_STATUS_IS_OK(status)) {
			goto fail;
		}
	}

	if (!fsp->is_directory && S_ISDIR(fsp->fsp_name->st.st_ex_mode)) {
		status = NT_STATUS_ACCESS_DENIED;
		goto fail;
	}

	/* Save the requested allocation size. */
	if ((info == FILE_WAS_CREATED) || (info == FILE_WAS_OVERWRITTEN)) {
		if (allocation_size
		    && (allocation_size > fsp->fsp_name->st.st_ex_size)) {
			fsp->initial_allocation_size = smb_roundup(
				fsp->conn, allocation_size);
			if (fsp->is_directory) {
				/* Can't set allocation size on a directory. */
				status = NT_STATUS_ACCESS_DENIED;
				goto fail;
			}
			if (vfs_allocate_file_space(
				    fsp, fsp->initial_allocation_size) == -1) {
				status = NT_STATUS_DISK_FULL;
				goto fail;
			}
		} else {
			fsp->initial_allocation_size = smb_roundup(
				fsp->conn, (uint64_t)fsp->fsp_name->st.st_ex_size);
		}
	} else {
		fsp->initial_allocation_size = 0;
	}

	if ((info == FILE_WAS_CREATED) && lp_nt_acl_support(SNUM(conn)) &&
				fsp->base_fsp == NULL) {
		if (sd != NULL) {
			/*
			 * According to the MS documentation, the only time the security
			 * descriptor is applied to the opened file is iff we *created* the
			 * file; an existing file stays the same.
			 *
			 * Also, it seems (from observation) that you can open the file with
			 * any access mask but you can still write the sd. We need to override
			 * the granted access before we call set_sd
			 * Patch for bug #2242 from Tom Lackemann <cessnatomny@yahoo.com>.
			 */

			uint32_t sec_info_sent;
			uint32_t saved_access_mask = fsp->access_mask;

			sec_info_sent = get_sec_info(sd);

			fsp->access_mask = FILE_GENERIC_ALL;

			if (sec_info_sent & (SECINFO_OWNER|
						SECINFO_GROUP|
						SECINFO_DACL|
						SECINFO_SACL)) {
				status = set_sd(fsp, sd, sec_info_sent);
			}

			fsp->access_mask = saved_access_mask;

			if (!NT_STATUS_IS_OK(status)) {
				goto fail;
			}
		} else if (lp_inherit_acls(SNUM(conn))) {
			/* Inherit from parent. Errors here are not fatal. */
			status = inherit_new_acl(fsp);
			if (!NT_STATUS_IS_OK(status)) {
				DEBUG(10,("inherit_new_acl: failed for %s with %s\n",
					fsp_str_dbg(fsp),
					nt_errstr(status) ));
			}
		}
	}

	DEBUG(10, ("create_file_unixpath: info=%d\n", info));

	*result = fsp;
	if (pinfo != NULL) {
		*pinfo = info;
	}

	smb_fname->st = fsp->fsp_name->st;

	return NT_STATUS_OK;

 fail:
	DEBUG(10, ("create_file_unixpath: %s\n", nt_errstr(status)));

	if (fsp != NULL) {
		if (base_fsp && fsp->base_fsp == base_fsp) {
			/*
			 * The close_file below will close
			 * fsp->base_fsp.
			 */
			base_fsp = NULL;
		}
		close_file(req, fsp, ERROR_CLOSE);
		fsp = NULL;
	}
	if (base_fsp != NULL) {
		close_file(req, base_fsp, ERROR_CLOSE);
		base_fsp = NULL;
	}
	return status;
}