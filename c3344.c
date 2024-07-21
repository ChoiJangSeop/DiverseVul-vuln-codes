void wait_for_receiver(void)
{
	if (!iobuf.raw_input_ends_before)
		read_a_msg();

	if (iobuf.raw_input_ends_before) {
		int ndx = read_int(iobuf.in_fd);
		if (ndx < 0) {
			switch (ndx) {
			case NDX_FLIST_EOF:
				flist_eof = 1;
				if (DEBUG_GTE(FLIST, 3))
					rprintf(FINFO, "[%s] flist_eof=1\n", who_am_i());
				break;
			case NDX_DONE:
				msgdone_cnt++;
				break;
			default:
				exit_cleanup(RERR_STREAMIO);
			}
		} else {
			struct file_list *flist;
			flist_receiving_enabled = False;
			if (DEBUG_GTE(FLIST, 2)) {
				rprintf(FINFO, "[%s] receiving flist for dir %d\n",
					who_am_i(), ndx);
			}
			flist = recv_file_list(iobuf.in_fd);
			flist->parent_ndx = ndx;
#ifdef SUPPORT_HARD_LINKS
			if (preserve_hard_links)
				match_hard_links(flist);
#endif
			flist_receiving_enabled = True;
		}
	}
}