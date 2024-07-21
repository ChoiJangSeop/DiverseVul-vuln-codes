NTSTATUS check_reduced_name(connection_struct *conn, const char *fname)
{
#ifdef REALPATH_TAKES_NULL
	bool free_resolved_name = True;
#else
        char resolved_name_buf[PATH_MAX+1];
	bool free_resolved_name = False;
#endif
	char *resolved_name = NULL;
	char *p = NULL;

	DEBUG(3,("check_reduced_name [%s] [%s]\n", fname, conn->connectpath));

#ifdef REALPATH_TAKES_NULL
	resolved_name = SMB_VFS_REALPATH(conn,fname,NULL);
#else
	resolved_name = SMB_VFS_REALPATH(conn,fname,resolved_name_buf);
#endif

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
				char *tmp_fname = NULL;
				char *last_component = NULL;
				/* Last component didn't exist. Remove it and try and canonicalise the directory. */

				tmp_fname = talloc_strdup(ctx, fname);
				if (!tmp_fname) {
					return NT_STATUS_NO_MEMORY;
				}
				p = strrchr_m(tmp_fname, '/');
				if (p) {
					*p++ = '\0';
					last_component = p;
				} else {
					last_component = tmp_fname;
					tmp_fname = talloc_strdup(ctx,
							".");
					if (!tmp_fname) {
						return NT_STATUS_NO_MEMORY;
					}
				}

#ifdef REALPATH_TAKES_NULL
				resolved_name = SMB_VFS_REALPATH(conn,tmp_fname,NULL);
#else
				resolved_name = SMB_VFS_REALPATH(conn,tmp_fname,resolved_name_buf);
#endif
				if (!resolved_name) {
					NTSTATUS status = map_nt_error_from_unix(errno);

					if (errno == ENOENT || errno == ENOTDIR) {
						status = NT_STATUS_OBJECT_PATH_NOT_FOUND;
					}

					DEBUG(3,("check_reduce_named: "
						 "couldn't get realpath for "
						 "%s (%s)\n",
						fname,
						nt_errstr(status)));
					return status;
				}
				tmp_fname = talloc_asprintf(ctx,
						"%s/%s",
						resolved_name,
						last_component);
				if (!tmp_fname) {
					return NT_STATUS_NO_MEMORY;
				}
#ifdef REALPATH_TAKES_NULL
				SAFE_FREE(resolved_name);
				resolved_name = SMB_STRDUP(tmp_fname);
				if (!resolved_name) {
					DEBUG(0, ("check_reduced_name: malloc "
						  "fail for %s\n", tmp_fname));
					return NT_STATUS_NO_MEMORY;
				}
#else
				safe_strcpy(resolved_name_buf, tmp_fname, PATH_MAX);
				resolved_name = resolved_name_buf;
#endif
				break;
			}
			default:
				DEBUG(1,("check_reduced_name: couldn't get "
					 "realpath for %s\n", fname));
				return map_nt_error_from_unix(errno);
		}
	}

	DEBUG(10,("check_reduced_name realpath [%s] -> [%s]\n", fname,
		  resolved_name));

	if (*resolved_name != '/') {
		DEBUG(0,("check_reduced_name: realpath doesn't return "
			 "absolute paths !\n"));
		if (free_resolved_name) {
			SAFE_FREE(resolved_name);
		}
		return NT_STATUS_OBJECT_NAME_INVALID;
	}

	/* Check for widelinks allowed. */
	if (!lp_widelinks(SNUM(conn))) {
		    const char *conn_rootdir;

		    conn_rootdir = SMB_VFS_CONNECTPATH(conn, fname);
		    if (conn_rootdir == NULL) {
			    DEBUG(2, ("check_reduced_name: Could not get "
				      "conn_rootdir\n"));
			    if (free_resolved_name) {
				    SAFE_FREE(resolved_name);
			    }
			    return NT_STATUS_ACCESS_DENIED;
		    }

		    if (strncmp(conn_rootdir, resolved_name,
				strlen(conn_rootdir)) != 0) {
			    DEBUG(2, ("check_reduced_name: Bad access "
				      "attempt: %s is a symlink outside the "
				      "share path", fname));
			    if (free_resolved_name) {
				    SAFE_FREE(resolved_name);
			    }
			    return NT_STATUS_ACCESS_DENIED;
		    }
	}

        /* Check if we are allowing users to follow symlinks */
        /* Patch from David Clerc <David.Clerc@cui.unige.ch>
                University of Geneva */

#ifdef S_ISLNK
        if (!lp_symlinks(SNUM(conn))) {
		struct smb_filename *smb_fname = NULL;
		NTSTATUS status;

		status = create_synthetic_smb_fname(talloc_tos(), fname, NULL,
						    NULL, &smb_fname);
		if (!NT_STATUS_IS_OK(status)) {
			if (free_resolved_name) {
				SAFE_FREE(resolved_name);
			}
                        return status;
		}

		if ( (SMB_VFS_LSTAT(conn, smb_fname) != -1) &&
                                (S_ISLNK(smb_fname->st.st_ex_mode)) ) {
			if (free_resolved_name) {
				SAFE_FREE(resolved_name);
			}
                        DEBUG(3,("check_reduced_name: denied: file path name "
				 "%s is a symlink\n",resolved_name));
			TALLOC_FREE(smb_fname);
			return NT_STATUS_ACCESS_DENIED;
                }
		TALLOC_FREE(smb_fname);
        }
#endif

	DEBUG(3,("check_reduced_name: %s reduced to %s\n", fname,
		 resolved_name));
	if (free_resolved_name) {
		SAFE_FREE(resolved_name);
	}
	return NT_STATUS_OK;
}