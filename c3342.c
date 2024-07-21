void recv_additional_file_list(int f)
{
	struct file_list *flist;
	int ndx = read_ndx(f);
	if (ndx == NDX_FLIST_EOF) {
		flist_eof = 1;
		if (DEBUG_GTE(FLIST, 3))
			rprintf(FINFO, "[%s] flist_eof=1\n", who_am_i());
		change_local_filter_dir(NULL, 0, 0);
	} else {
		ndx = NDX_FLIST_OFFSET - ndx;
		if (ndx < 0 || ndx >= dir_flist->used) {
			ndx = NDX_FLIST_OFFSET - ndx;
			rprintf(FERROR,
				"[%s] Invalid dir index: %d (%d - %d)\n",
				who_am_i(), ndx, NDX_FLIST_OFFSET,
				NDX_FLIST_OFFSET - dir_flist->used + 1);
			exit_cleanup(RERR_PROTOCOL);
		}
		if (DEBUG_GTE(FLIST, 3)) {
			rprintf(FINFO, "[%s] receiving flist for dir %d\n",
				who_am_i(), ndx);
		}
		flist = recv_file_list(f);
		flist->parent_ndx = ndx;
	}
}