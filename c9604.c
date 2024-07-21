NTSTATUS change_notify_create(struct files_struct *fsp,
			      uint32_t max_buffer_size,
			      uint32_t filter,
			      bool recursive)
{
	size_t len = fsp_fullbasepath(fsp, NULL, 0);
	char fullpath[len+1];
	NTSTATUS status = NT_STATUS_NOT_IMPLEMENTED;

	if (fsp->notify != NULL) {
		DEBUG(1, ("change_notify_create: fsp->notify != NULL, "
			  "fname = %s\n", fsp->fsp_name->base_name));
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!(fsp->notify = talloc_zero(NULL, struct notify_change_buf))) {
		DEBUG(0, ("talloc failed\n"));
		return NT_STATUS_NO_MEMORY;
	}
	fsp->notify->filter = filter;
	fsp->notify->subdir_filter = recursive ? filter : 0;
	fsp->notify->max_buffer_size = max_buffer_size;

	fsp_fullbasepath(fsp, fullpath, sizeof(fullpath));

	/*
	 * Avoid /. at the end of the path name. notify can't deal with it.
	 */
	if (len > 1 && fullpath[len-1] == '.' && fullpath[len-2] == '/') {
		fullpath[len-2] = '\0';
	}

	if ((fsp->notify->filter != 0) ||
	    (fsp->notify->subdir_filter != 0)) {
		status = notify_add(fsp->conn->sconn->notify_ctx,
				    fullpath, fsp->notify->filter,
				    fsp->notify->subdir_filter, fsp);
	}

	return status;
}