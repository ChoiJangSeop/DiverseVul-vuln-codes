static NTSTATUS smb_set_file_unix_link(connection_struct *conn,
				       struct smb_request *req,
				       const char *pdata,
				       int total_data,
				       const struct smb_filename *smb_fname)
{
	char *link_target = NULL;
	const char *newname = smb_fname->base_name;
	NTSTATUS status = NT_STATUS_OK;
	TALLOC_CTX *ctx = talloc_tos();

	/* Set a symbolic link. */
	/* Don't allow this if follow links is false. */

	if (total_data == 0) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!lp_symlinks(SNUM(conn))) {
		return NT_STATUS_ACCESS_DENIED;
	}

	srvstr_pull_talloc(ctx, pdata, req->flags2, &link_target, pdata,
		    total_data, STR_TERMINATE);

	if (!link_target) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* !widelinks forces the target path to be within the share. */
	/* This means we can interpret the target as a pathname. */
	if (!lp_widelinks(SNUM(conn))) {
		char *rel_name = NULL;
		char *last_dirp = NULL;

		if (*link_target == '/') {
			/* No absolute paths allowed. */
			return NT_STATUS_ACCESS_DENIED;
		}
		rel_name = talloc_strdup(ctx,newname);
		if (!rel_name) {
			return NT_STATUS_NO_MEMORY;
		}
		last_dirp = strrchr_m(rel_name, '/');
		if (last_dirp) {
			last_dirp[1] = '\0';
		} else {
			rel_name = talloc_strdup(ctx,"./");
			if (!rel_name) {
				return NT_STATUS_NO_MEMORY;
			}
		}
		rel_name = talloc_asprintf_append(rel_name,
				"%s",
				link_target);
		if (!rel_name) {
			return NT_STATUS_NO_MEMORY;
		}

		status = check_name(conn, rel_name);
		if (!NT_STATUS_IS_OK(status)) {
			return status;
		}
	}

	DEBUG(10,("smb_set_file_unix_link: SMB_SET_FILE_UNIX_LINK doing symlink %s -> %s\n",
			newname, link_target ));

	if (SMB_VFS_SYMLINK(conn,link_target,newname) != 0) {
		return map_nt_error_from_unix(errno);
	}

	return NT_STATUS_OK;
}