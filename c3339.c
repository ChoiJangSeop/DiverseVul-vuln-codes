static void do_server_recv(int f_in, int f_out, int argc, char *argv[])
{
	int exit_code;
	struct file_list *flist;
	char *local_name = NULL;
	int negated_levels;

	if (filesfrom_fd >= 0 && !msgs2stderr && protocol_version < 31) {
		/* We can't mix messages with files-from data on the socket,
		 * so temporarily turn off info/debug messages. */
		negate_output_levels();
		negated_levels = 1;
	} else
		negated_levels = 0;

	if (DEBUG_GTE(RECV, 1))
		rprintf(FINFO, "server_recv(%d) starting pid=%d\n", argc, (int)getpid());

	if (am_daemon && read_only) {
		rprintf(FERROR,"ERROR: module is read only\n");
		exit_cleanup(RERR_SYNTAX);
		return;
	}

	if (argc > 0) {
		char *dir = argv[0];
		argc--;
		argv++;
		if (!am_daemon && !change_dir(dir, CD_NORMAL)) {
			rsyserr(FERROR, errno, "change_dir#4 %s failed",
				full_fname(dir));
			exit_cleanup(RERR_FILESELECT);
		}
	}

	if (protocol_version >= 30)
		io_start_multiplex_in(f_in);
	else
		io_start_buffering_in(f_in);
	recv_filter_list(f_in);

	if (filesfrom_fd >= 0) {
		/* We need to send the files-from names to the sender at the
		 * same time that we receive the file-list from them, so we
		 * need the IO routines to automatically write out the names
		 * onto our f_out socket as we read the file-list.  This
		 * avoids both deadlock and extra delays/buffers. */
		start_filesfrom_forwarding(filesfrom_fd);
		filesfrom_fd = -1;
	}

	flist = recv_file_list(f_in);
	if (!flist) {
		rprintf(FERROR,"server_recv: recv_file_list error\n");
		exit_cleanup(RERR_FILESELECT);
	}
	if (inc_recurse && file_total == 1)
		recv_additional_file_list(f_in);

	if (negated_levels)
		negate_output_levels();

	if (argc > 0)
		local_name = get_local_name(flist,argv[0]);

	/* Now that we know what our destination directory turned out to be,
	 * we can sanitize the --link-/copy-/compare-dest args correctly. */
	if (sanitize_paths) {
		char **dir_p;
		for (dir_p = basis_dir; *dir_p; dir_p++)
			*dir_p = sanitize_path(NULL, *dir_p, NULL, curr_dir_depth, SP_DEFAULT);
		if (partial_dir)
			partial_dir = sanitize_path(NULL, partial_dir, NULL, curr_dir_depth, SP_DEFAULT);
	}
	check_alt_basis_dirs();

	if (daemon_filter_list.head) {
		char **dir_p;
		filter_rule_list *elp = &daemon_filter_list;

		for (dir_p = basis_dir; *dir_p; dir_p++) {
			char *dir = *dir_p;
			if (*dir == '/')
				dir += module_dirlen;
			if (check_filter(elp, FLOG, dir, 1) < 0)
				goto options_rejected;
		}
		if (partial_dir && *partial_dir == '/'
		 && check_filter(elp, FLOG, partial_dir + module_dirlen, 1) < 0) {
		    options_rejected:
			rprintf(FERROR,
				"Your options have been rejected by the server.\n");
			exit_cleanup(RERR_SYNTAX);
		}
	}

	exit_code = do_recv(f_in, f_out, local_name);
	exit_cleanup(exit_code);
}