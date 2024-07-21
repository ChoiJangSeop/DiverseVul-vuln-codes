NTSTATUS check_reduced_name(connection_struct *conn, const char *fname)
{
	char *resolved_name = NULL;
	bool allow_symlinks = true;
	bool allow_widelinks = false;

	DBG_DEBUG("check_reduced_name [%s] [%s]\n", fname, conn->connectpath);

	resolved_name = SMB_VFS_REALPATH(conn,fname);

	if (!resolved_name) {
		switch (errno) {
			case ENOTDIR:
				DEBUG(3,("check_reduced_name: Component not a "
					 "directory in getting realpath for "
					 "%s\n", fname));
				return NT_STATUS_OBJECT_PATH_NOT_FOUND;
			case ENOENT:
			{
				TALLOC_CTX *ctx = talloc_tos();
				char *dir_name = NULL;
				const char *last_component = NULL;
				char *new_name = NULL;
				int ret;

				/* Last component didn't exist.
				   Remove it and try and canonicalise
				   the directory name. */
				if (!parent_dirname(ctx, fname,
						&dir_name,
						&last_component)) {
					return NT_STATUS_NO_MEMORY;
				}

				resolved_name = SMB_VFS_REALPATH(conn,dir_name);
				if (!resolved_name) {
					NTSTATUS status = map_nt_error_from_unix(errno);

					if (errno == ENOENT || errno == ENOTDIR) {
						status = NT_STATUS_OBJECT_PATH_NOT_FOUND;
					}

					DEBUG(3,("check_reduce_name: "
						 "couldn't get realpath for "
						 "%s (%s)\n",
						fname,
						nt_errstr(status)));
					return status;
				}
				ret = asprintf(&new_name, "%s/%s",
					       resolved_name, last_component);
				SAFE_FREE(resolved_name);
				if (ret == -1) {
					return NT_STATUS_NO_MEMORY;
				}
				resolved_name = new_name;
				break;
			}
			default:
				DEBUG(3,("check_reduced_name: couldn't get "
					 "realpath for %s\n", fname));
				return map_nt_error_from_unix(errno);
		}
	}

	DEBUG(10,("check_reduced_name realpath [%s] -> [%s]\n", fname,
		  resolved_name));

	if (*resolved_name != '/') {
		DEBUG(0,("check_reduced_name: realpath doesn't return "
			 "absolute paths !\n"));
		SAFE_FREE(resolved_name);
		return NT_STATUS_OBJECT_NAME_INVALID;
	}

	allow_widelinks = lp_widelinks(SNUM(conn));
	allow_symlinks = lp_follow_symlinks(SNUM(conn));

	/* Common widelinks and symlinks checks. */
	if (!allow_widelinks || !allow_symlinks) {
		const char *conn_rootdir;
		size_t rootdir_len;

		conn_rootdir = SMB_VFS_CONNECTPATH(conn, fname);
		if (conn_rootdir == NULL) {
			DEBUG(2, ("check_reduced_name: Could not get "
				"conn_rootdir\n"));
			SAFE_FREE(resolved_name);
			return NT_STATUS_ACCESS_DENIED;
		}

		rootdir_len = strlen(conn_rootdir);
		if (strncmp(conn_rootdir, resolved_name,
				rootdir_len) != 0) {
			DEBUG(2, ("check_reduced_name: Bad access "
				"attempt: %s is a symlink outside the "
				"share path\n", fname));
			DEBUGADD(2, ("conn_rootdir =%s\n", conn_rootdir));
			DEBUGADD(2, ("resolved_name=%s\n", resolved_name));
			SAFE_FREE(resolved_name);
			return NT_STATUS_ACCESS_DENIED;
		}

		/* Extra checks if all symlinks are disallowed. */
		if (!allow_symlinks) {
			/* fname can't have changed in resolved_path. */
			const char *p = &resolved_name[rootdir_len];

			/* *p can be '\0' if fname was "." */
			if (*p == '\0' && ISDOT(fname)) {
				goto out;
			}

			if (*p != '/') {
				DEBUG(2, ("check_reduced_name: logic error (%c) "
					"in resolved_name: %s\n",
					*p,
					fname));
				SAFE_FREE(resolved_name);
				return NT_STATUS_ACCESS_DENIED;
			}

			p++;
			if (strcmp(fname, p)!=0) {
				DEBUG(2, ("check_reduced_name: Bad access "
					"attempt: %s is a symlink to %s\n",
					  fname, p));
				SAFE_FREE(resolved_name);
				return NT_STATUS_ACCESS_DENIED;
			}
		}
	}

  out:

	DBG_INFO("%s reduced to %s\n", fname, resolved_name);
	SAFE_FREE(resolved_name);
	return NT_STATUS_OK;
}