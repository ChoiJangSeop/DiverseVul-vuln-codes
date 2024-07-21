void setup_protocol(int f_out,int f_in)
{
	if (am_sender)
		file_extra_cnt += PTR_EXTRA_CNT;
	else
		file_extra_cnt++;
	if (preserve_uid)
		uid_ndx = ++file_extra_cnt;
	if (preserve_gid)
		gid_ndx = ++file_extra_cnt;
	if (preserve_acls && !am_sender)
		acls_ndx = ++file_extra_cnt;
	if (preserve_xattrs)
		xattrs_ndx = ++file_extra_cnt;

	if (am_server)
		set_allow_inc_recurse();

	if (remote_protocol == 0) {
		if (am_server && !local_server)
			check_sub_protocol();
		if (!read_batch)
			write_int(f_out, protocol_version);
		remote_protocol = read_int(f_in);
		if (protocol_version > remote_protocol)
			protocol_version = remote_protocol;
	}
	if (read_batch && remote_protocol > protocol_version) {
		rprintf(FERROR, "The protocol version in the batch file is too new (%d > %d).\n",
			remote_protocol, protocol_version);
		exit_cleanup(RERR_PROTOCOL);
	}

	if (DEBUG_GTE(PROTO, 1)) {
		rprintf(FINFO, "(%s) Protocol versions: remote=%d, negotiated=%d\n",
			am_server? "Server" : "Client", remote_protocol, protocol_version);
	}
	if (remote_protocol < MIN_PROTOCOL_VERSION
	 || remote_protocol > MAX_PROTOCOL_VERSION) {
		rprintf(FERROR,"protocol version mismatch -- is your shell clean?\n");
		rprintf(FERROR,"(see the rsync man page for an explanation)\n");
		exit_cleanup(RERR_PROTOCOL);
	}
	if (remote_protocol < OLD_PROTOCOL_VERSION) {
		rprintf(FINFO,"%s is very old version of rsync, upgrade recommended.\n",
			am_server? "Client" : "Server");
	}
	if (protocol_version < MIN_PROTOCOL_VERSION) {
		rprintf(FERROR, "--protocol must be at least %d on the %s.\n",
			MIN_PROTOCOL_VERSION, am_server? "Server" : "Client");
		exit_cleanup(RERR_PROTOCOL);
	}
	if (protocol_version > PROTOCOL_VERSION) {
		rprintf(FERROR, "--protocol must be no more than %d on the %s.\n",
			PROTOCOL_VERSION, am_server? "Server" : "Client");
		exit_cleanup(RERR_PROTOCOL);
	}
	if (read_batch)
		check_batch_flags();

#ifndef SUPPORT_PREALLOCATION
	if (preallocate_files && !am_sender) {
		rprintf(FERROR, "preallocation is not supported on this %s\n",
			am_server ? "Server" : "Client");
		exit_cleanup(RERR_SYNTAX);
	}
#endif

	if (protocol_version < 30) {
		if (append_mode == 1)
			append_mode = 2;
		if (preserve_acls && !local_server) {
			rprintf(FERROR,
			    "--acls requires protocol 30 or higher"
			    " (negotiated %d).\n",
			    protocol_version);
			exit_cleanup(RERR_PROTOCOL);
		}
		if (preserve_xattrs && !local_server) {
			rprintf(FERROR,
			    "--xattrs requires protocol 30 or higher"
			    " (negotiated %d).\n",
			    protocol_version);
			exit_cleanup(RERR_PROTOCOL);
		}
	}

	if (delete_mode && !(delete_before+delete_during+delete_after)) {
		if (protocol_version < 30)
			delete_before = 1;
		else
			delete_during = 1;
	}

	if (protocol_version < 29) {
		if (fuzzy_basis) {
			rprintf(FERROR,
			    "--fuzzy requires protocol 29 or higher"
			    " (negotiated %d).\n",
			    protocol_version);
			exit_cleanup(RERR_PROTOCOL);
		}

		if (basis_dir_cnt && inplace) {
			rprintf(FERROR,
			    "%s with --inplace requires protocol 29 or higher"
			    " (negotiated %d).\n",
			    dest_option, protocol_version);
			exit_cleanup(RERR_PROTOCOL);
		}

		if (basis_dir_cnt > 1) {
			rprintf(FERROR,
			    "Using more than one %s option requires protocol"
			    " 29 or higher (negotiated %d).\n",
			    dest_option, protocol_version);
			exit_cleanup(RERR_PROTOCOL);
		}

		if (prune_empty_dirs) {
			rprintf(FERROR,
			    "--prune-empty-dirs requires protocol 29 or higher"
			    " (negotiated %d).\n",
			    protocol_version);
			exit_cleanup(RERR_PROTOCOL);
		}
	} else if (protocol_version >= 30) {
		if (am_server) {
			compat_flags = allow_inc_recurse ? CF_INC_RECURSE : 0;
#ifdef CAN_SET_SYMLINK_TIMES
			compat_flags |= CF_SYMLINK_TIMES;
#endif
#ifdef ICONV_OPTION
			compat_flags |= CF_SYMLINK_ICONV;
#endif
			if (local_server || strchr(client_info, 'f') != NULL)
				compat_flags |= CF_SAFE_FLIST;
			if (local_server || strchr(client_info, 'x') != NULL)
				compat_flags |= CF_AVOID_XATTR_OPTIM;
			write_byte(f_out, compat_flags);
		} else
			compat_flags = read_byte(f_in);
		/* The inc_recurse var MUST be set to 0 or 1. */
		inc_recurse = compat_flags & CF_INC_RECURSE ? 1 : 0;
		want_xattr_optim = protocol_version >= 31 && !(compat_flags & CF_AVOID_XATTR_OPTIM);
		if (am_sender) {
			receiver_symlink_times = am_server
			    ? strchr(client_info, 'L') != NULL
			    : !!(compat_flags & CF_SYMLINK_TIMES);
		}
#ifdef CAN_SET_SYMLINK_TIMES
		else
			receiver_symlink_times = 1;
#endif
#ifdef ICONV_OPTION
		sender_symlink_iconv = iconv_opt && (am_server
		    ? local_server || strchr(client_info, 's') != NULL
		    : !!(compat_flags & CF_SYMLINK_ICONV));
#endif
		if (inc_recurse && !allow_inc_recurse) {
			/* This should only be able to happen in a batch. */
			fprintf(stderr,
			    "Incompatible options specified for inc-recursive %s.\n",
			    read_batch ? "batch file" : "connection");
			exit_cleanup(RERR_SYNTAX);
		}
		use_safe_inc_flist = (compat_flags & CF_SAFE_FLIST) || protocol_version >= 31;
		need_messages_from_generator = 1;
#ifdef CAN_SET_SYMLINK_TIMES
	} else if (!am_sender) {
		receiver_symlink_times = 1;
#endif
	}

	if (need_unsorted_flist && (!am_sender || inc_recurse))
		unsort_ndx = ++file_extra_cnt;

	if (partial_dir && *partial_dir != '/' && (!am_server || local_server)) {
		int rflags = FILTRULE_NO_PREFIXES | FILTRULE_DIRECTORY;
		if (!am_sender || protocol_version >= 30)
			rflags |= FILTRULE_PERISHABLE;
		parse_filter_str(&filter_list, partial_dir, rule_template(rflags), 0);
	}


#ifdef ICONV_OPTION
	if (protect_args && files_from) {
		if (am_sender)
			filesfrom_convert = filesfrom_host && ic_send != (iconv_t)-1;
		else
			filesfrom_convert = !filesfrom_host && ic_recv != (iconv_t)-1;
	}
#endif

	if (am_server) {
		if (!checksum_seed)
			checksum_seed = time(NULL);
		write_int(f_out, checksum_seed);
	} else {
		checksum_seed = read_int(f_in);
	}
}