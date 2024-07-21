struct file_list *recv_file_list(int f)
{
	struct file_list *flist;
	int dstart, flags;
	int64 start_read;

	if (!first_flist) {
		if (show_filelist_p())
			start_filelist_progress("receiving file list");
		else if (inc_recurse && INFO_GTE(FLIST, 1) && !am_server)
			rprintf(FCLIENT, "receiving incremental file list\n");
		rprintf(FLOG, "receiving file list\n");
		if (usermap)
			parse_name_map(usermap, True);
		if (groupmap)
			parse_name_map(groupmap, False);
	}

	start_read = stats.total_read;

#ifdef SUPPORT_HARD_LINKS
	if (preserve_hard_links && !first_flist)
		init_hard_links();
#endif

	flist = flist_new(0, "recv_file_list");

	if (inc_recurse) {
		if (flist->ndx_start == 1)
			dir_flist = flist_new(FLIST_TEMP, "recv_file_list");
		dstart = dir_flist->used;
	} else {
		dir_flist = flist;
		dstart = 0;
	}

	while ((flags = read_byte(f)) != 0) {
		struct file_struct *file;

		if (protocol_version >= 28 && (flags & XMIT_EXTENDED_FLAGS))
			flags |= read_byte(f) << 8;

		if (flags == (XMIT_EXTENDED_FLAGS|XMIT_IO_ERROR_ENDLIST)) {
			int err;
			if (!use_safe_inc_flist) {
				rprintf(FERROR, "Invalid flist flag: %x\n", flags);
				exit_cleanup(RERR_PROTOCOL);
			}
			err = read_varint(f);
			if (!ignore_errors)
				io_error |= err;
			break;
		}

		flist_expand(flist, 1);
		file = recv_file_entry(f, flist, flags);

		if (S_ISREG(file->mode)) {
			/* Already counted */
		} else if (S_ISDIR(file->mode)) {
			if (inc_recurse) {
				flist_expand(dir_flist, 1);
				dir_flist->files[dir_flist->used++] = file;
			}
			stats.num_dirs++;
		} else if (S_ISLNK(file->mode))
			stats.num_symlinks++;
		else if (IS_DEVICE(file->mode))
			stats.num_symlinks++;
		else
			stats.num_specials++;

		flist->files[flist->used++] = file;

		maybe_emit_filelist_progress(flist->used);

		if (DEBUG_GTE(FLIST, 2)) {
			char *name = f_name(file, NULL);
			rprintf(FINFO, "recv_file_name(%s)\n", NS(name));
		}
	}
	file_total += flist->used;

	if (DEBUG_GTE(FLIST, 2))
		rprintf(FINFO, "received %d names\n", flist->used);

	if (show_filelist_p())
		finish_filelist_progress(flist);

	if (need_unsorted_flist) {
		/* Create an extra array of index pointers that we can sort for
		 * the generator's use (for wading through the files in sorted
		 * order and for calling flist_find()).  We keep the "files"
		 * list unsorted for our exchange of index numbers with the
		 * other side (since their names may not sort the same). */
		if (!(flist->sorted = new_array(struct file_struct *, flist->used)))
			out_of_memory("recv_file_list");
		memcpy(flist->sorted, flist->files,
		       flist->used * sizeof (struct file_struct*));
		if (inc_recurse && dir_flist->used > dstart) {
			static int dir_flist_malloced = 0;
			if (dir_flist_malloced < dir_flist->malloced) {
				dir_flist->sorted = realloc_array(dir_flist->sorted,
							struct file_struct *,
							dir_flist->malloced);
				dir_flist_malloced = dir_flist->malloced;
			}
			memcpy(dir_flist->sorted + dstart, dir_flist->files + dstart,
			       (dir_flist->used - dstart) * sizeof (struct file_struct*));
			fsort(dir_flist->sorted + dstart, dir_flist->used - dstart);
		}
	} else {
		flist->sorted = flist->files;
		if (inc_recurse && dir_flist->used > dstart) {
			dir_flist->sorted = dir_flist->files;
			fsort(dir_flist->sorted + dstart, dir_flist->used - dstart);
		}
	}

	if (inc_recurse)
		flist_done_allocating(flist);
	else if (f >= 0) {
		recv_id_list(f, flist);
		flist_eof = 1;
		if (DEBUG_GTE(FLIST, 3))
			rprintf(FINFO, "[%s] flist_eof=1\n", who_am_i());
	}

	flist_sort_and_clean(flist, relative_paths);

	if (protocol_version < 30) {
		/* Recv the io_error flag */
		int err = read_int(f);
		if (!ignore_errors)
			io_error |= err;
	} else if (inc_recurse && flist->ndx_start == 1) {
		if (!file_total || strcmp(flist->sorted[flist->low]->basename, ".") != 0)
			flist->parent_ndx = -1;
	}

	if (DEBUG_GTE(FLIST, 3))
		output_flist(flist);

	if (DEBUG_GTE(FLIST, 2))
		rprintf(FINFO, "recv_file_list done\n");

	stats.flist_size += stats.total_read - start_read;
	stats.num_files += flist->used;

	return flist;
}