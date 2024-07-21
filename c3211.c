static int shadow_copy2_get_shadow_copy_data(
	vfs_handle_struct *handle, files_struct *fsp,
	struct shadow_copy_data *shadow_copy2_data,
	bool labels)
{
	DIR *p;
	const char *snapdir;
	struct dirent *d;
	TALLOC_CTX *tmp_ctx = talloc_stackframe();

	snapdir = shadow_copy2_find_snapdir(tmp_ctx, handle, fsp->fsp_name);
	if (snapdir == NULL) {
		DEBUG(0,("shadow:snapdir not found for %s in get_shadow_copy_data\n",
			 handle->conn->connectpath));
		errno = EINVAL;
		talloc_free(tmp_ctx);
		return -1;
	}

	p = SMB_VFS_NEXT_OPENDIR(handle, snapdir, NULL, 0);

	if (!p) {
		DEBUG(2,("shadow_copy2: SMB_VFS_NEXT_OPENDIR() failed for '%s'"
			 " - %s\n", snapdir, strerror(errno)));
		talloc_free(tmp_ctx);
		errno = ENOSYS;
		return -1;
	}

	shadow_copy2_data->num_volumes = 0;
	shadow_copy2_data->labels      = NULL;

	while ((d = SMB_VFS_NEXT_READDIR(handle, p, NULL))) {
		char snapshot[GMT_NAME_LEN+1];
		SHADOW_COPY_LABEL *tlabels;

		/*
		 * ignore names not of the right form in the snapshot
		 * directory
		 */
		if (!shadow_copy2_snapshot_to_gmt(
			    handle, d->d_name,
			    snapshot, sizeof(snapshot))) {

			DEBUG(6, ("shadow_copy2_get_shadow_copy_data: "
				  "ignoring %s\n", d->d_name));
			continue;
		}
		DEBUG(6,("shadow_copy2_get_shadow_copy_data: %s -> %s\n",
			 d->d_name, snapshot));

		if (!labels) {
			/* the caller doesn't want the labels */
			shadow_copy2_data->num_volumes++;
			continue;
		}

		tlabels = talloc_realloc(shadow_copy2_data,
					 shadow_copy2_data->labels,
					 SHADOW_COPY_LABEL,
					 shadow_copy2_data->num_volumes+1);
		if (tlabels == NULL) {
			DEBUG(0,("shadow_copy2: out of memory\n"));
			SMB_VFS_NEXT_CLOSEDIR(handle, p);
			talloc_free(tmp_ctx);
			return -1;
		}

		strlcpy(tlabels[shadow_copy2_data->num_volumes], snapshot,
			sizeof(*tlabels));

		shadow_copy2_data->num_volumes++;
		shadow_copy2_data->labels = tlabels;
	}

	SMB_VFS_NEXT_CLOSEDIR(handle,p);

	shadow_copy2_sort_data(handle, shadow_copy2_data);

	talloc_free(tmp_ctx);
	return 0;
}