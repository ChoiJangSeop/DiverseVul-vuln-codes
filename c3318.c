NTSTATUS check_reduced_name_with_privilege(connection_struct *conn,
			const char *fname,
			struct smb_request *smbreq)
{
	NTSTATUS status;
	TALLOC_CTX *ctx = talloc_tos();
	const char *conn_rootdir;
	size_t rootdir_len;
	char *dir_name = NULL;
	const char *last_component = NULL;
	char *resolved_name = NULL;
	char *saved_dir = NULL;
	struct smb_filename *smb_fname_cwd = NULL;
	struct privilege_paths *priv_paths = NULL;
	int ret;

	DEBUG(3,("check_reduced_name_with_privilege [%s] [%s]\n",
			fname,
			conn->connectpath));


	priv_paths = talloc_zero(smbreq, struct privilege_paths);
	if (!priv_paths) {
		status = NT_STATUS_NO_MEMORY;
		goto err;
	}

	if (!parent_dirname(ctx, fname, &dir_name, &last_component)) {
		status = NT_STATUS_NO_MEMORY;
		goto err;
	}

	priv_paths->parent_name.base_name = talloc_strdup(priv_paths, dir_name);
	priv_paths->file_name.base_name = talloc_strdup(priv_paths, last_component);

	if (priv_paths->parent_name.base_name == NULL ||
			priv_paths->file_name.base_name == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto err;
	}

	if (SMB_VFS_STAT(conn, &priv_paths->parent_name) != 0) {
		status = map_nt_error_from_unix(errno);
		goto err;
	}
	/* Remember where we were. */
	saved_dir = vfs_GetWd(ctx, conn);
	if (!saved_dir) {
		status = map_nt_error_from_unix(errno);
		goto err;
	}

	/* Go to the parent directory to lock in memory. */
	if (vfs_ChDir(conn, priv_paths->parent_name.base_name) == -1) {
		status = map_nt_error_from_unix(errno);
		goto err;
	}

	/* Get the absolute path of the parent directory. */
	resolved_name = SMB_VFS_REALPATH(conn,".");
	if (!resolved_name) {
		status = map_nt_error_from_unix(errno);
		goto err;
	}

	if (*resolved_name != '/') {
		DEBUG(0,("check_reduced_name_with_privilege: realpath "
			"doesn't return absolute paths !\n"));
		status = NT_STATUS_OBJECT_NAME_INVALID;
		goto err;
	}

	DEBUG(10,("check_reduced_name_with_privilege: realpath [%s] -> [%s]\n",
		priv_paths->parent_name.base_name,
		resolved_name));

	/* Now check the stat value is the same. */
	smb_fname_cwd = synthetic_smb_fname(talloc_tos(), ".", NULL, NULL);
	if (smb_fname_cwd == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto err;
	}

	if (SMB_VFS_LSTAT(conn, smb_fname_cwd) != 0) {
		status = map_nt_error_from_unix(errno);
		goto err;
	}

	/* Ensure we're pointing at the same place. */
	if (!check_same_stat(&smb_fname_cwd->st, &priv_paths->parent_name.st)) {
		DEBUG(0,("check_reduced_name_with_privilege: "
			"device/inode/uid/gid on directory %s changed. "
			"Denying access !\n",
			priv_paths->parent_name.base_name));
		status = NT_STATUS_ACCESS_DENIED;
		goto err;
	}

	/* Ensure we're below the connect path. */

	conn_rootdir = SMB_VFS_CONNECTPATH(conn, fname);
	if (conn_rootdir == NULL) {
		DEBUG(2, ("check_reduced_name_with_privilege: Could not get "
			"conn_rootdir\n"));
		status = NT_STATUS_ACCESS_DENIED;
		goto err;
	}

	rootdir_len = strlen(conn_rootdir);
	if (strncmp(conn_rootdir, resolved_name, rootdir_len) != 0) {
		DEBUG(2, ("check_reduced_name_with_privilege: Bad access "
			"attempt: %s is a symlink outside the "
			"share path\n",
			dir_name));
		DEBUGADD(2, ("conn_rootdir =%s\n", conn_rootdir));
		DEBUGADD(2, ("resolved_name=%s\n", resolved_name));
		status = NT_STATUS_ACCESS_DENIED;
		goto err;
	}

	/* Now ensure that the last component either doesn't
	   exist, or is *NOT* a symlink. */

	ret = SMB_VFS_LSTAT(conn, &priv_paths->file_name);
	if (ret == -1) {
		/* Errno must be ENOENT for this be ok. */
		if (errno != ENOENT) {
			status = map_nt_error_from_unix(errno);
			DEBUG(2, ("check_reduced_name_with_privilege: "
				"LSTAT on %s failed with %s\n",
				priv_paths->file_name.base_name,
				nt_errstr(status)));
			goto err;
		}
	}

	if (VALID_STAT(priv_paths->file_name.st) &&
			S_ISLNK(priv_paths->file_name.st.st_ex_mode)) {
		DEBUG(2, ("check_reduced_name_with_privilege: "
			"Last component %s is a symlink. Denying"
			"access.\n",
			priv_paths->file_name.base_name));
		status = NT_STATUS_ACCESS_DENIED;
		goto err;
	}

	smbreq->priv_paths = priv_paths;
	status = NT_STATUS_OK;

  err:

	if (saved_dir) {
		vfs_ChDir(conn, saved_dir);
	}
	SAFE_FREE(resolved_name);
	if (!NT_STATUS_IS_OK(status)) {
		TALLOC_FREE(priv_paths);
	}
	TALLOC_FREE(dir_name);
	return status;
}