void read_stats_from_file(char from_file[])
{
	struct file_magic file_magic;
	struct file_activity *file_actlst = NULL;
	char rec_hdr_tmp[MAX_RECORD_HEADER_SIZE];
	int curr = 1, i, p;
	int ifd, rtype;
	int rows, eosaf = TRUE, reset = FALSE;
	long cnt = 1;
	off_t fpos;

	/* Get window size */
	rows = get_win_height();

	/* Read file headers and activity list */
	check_file_actlst(&ifd, from_file, act, &file_magic, &file_hdr,
			  &file_actlst, id_seq, FALSE, &endian_mismatch, &arch_64);

	/* Perform required allocations */
	allocate_structures(act);

	/* Print report header */
	print_report_hdr(flags, &rectime, &file_hdr);

	/* Read system statistics from file */
	do {
		/*
		 * If this record is a special (RESTART or COMMENT) one, print it and
		 * (try to) get another one.
		 */
		do {
			if (read_record_hdr(ifd, rec_hdr_tmp, &record_hdr[0], &file_hdr,
					    arch_64, endian_mismatch, UEOF_STOP)) {
				/* End of sa data file */
				return;
			}

			rtype = record_hdr[0].record_type;
			if ((rtype == R_RESTART) || (rtype == R_COMMENT)) {
				print_special_record(&record_hdr[0], flags + S_F_LOCAL_TIME,
						     &tm_start, &tm_end, rtype, ifd,
						     &rectime, NULL, from_file, 0, &file_magic,
						     &file_hdr, act, &sar_fmt, endian_mismatch, arch_64);
			}
			else {
				/*
				 * OK: Previous record was not a special one.
				 * So read now the extra fields.
				 */
				read_file_stat_bunch(act, 0, ifd, file_hdr.sa_act_nr,
						     file_actlst, endian_mismatch, arch_64,
						     from_file, &file_magic, UEOF_STOP);
				if (sa_get_record_timestamp_struct(flags + S_F_LOCAL_TIME,
								   &record_hdr[0],
								   &rectime, NULL))
					/*
					 * An error was detected.
					 * The timestamp hasn't been updated.
					 */
					continue;
			}
		}
		while ((rtype == R_RESTART) || (rtype == R_COMMENT) ||
		       (tm_start.use && (datecmp(&rectime, &tm_start) < 0)) ||
		       (tm_end.use && (datecmp(&rectime, &tm_end) >=0)));

		/* Save the first stats collected. Will be used to compute the average */
		copy_structures(act, id_seq, record_hdr, 2, 0);

		reset = TRUE;	/* Set flag to reset last_uptime variable */

		/* Save current file position */
		if ((fpos = lseek(ifd, 0, SEEK_CUR)) < 0) {
			perror("lseek");
			exit(2);
		}

		/*
		 * Read and write stats located between two possible Linux restarts.
		 * Activities that should be displayed are saved in id_seq[] array.
		 * Since we are reading from a file, we print all the stats for an
		 * activity before displaying the next activity.
		 * id_seq[] has been created in check_file_actlst(), retaining only
		 * activities known by current sysstat version.
		 */
		for (i = 0; i < NR_ACT; i++) {

			if (!id_seq[i])
				continue;

			p = get_activity_position(act, id_seq[i], EXIT_IF_NOT_FOUND);
			if (!IS_SELECTED(act[p]->options))
				continue;

			if (!HAS_MULTIPLE_OUTPUTS(act[p]->options)) {
				handle_curr_act_stats(ifd, fpos, &curr, &cnt, &eosaf, rows,
						      act[p]->id, &reset, file_actlst,
						      from_file, &file_magic, rec_hdr_tmp,
						      endian_mismatch, arch_64);
			}
			else {
				unsigned int optf, msk;

				optf = act[p]->opt_flags;

				for (msk = 1; msk < 0x100; msk <<= 1) {
					if ((act[p]->opt_flags & 0xff) & msk) {
						act[p]->opt_flags &= (0xffffff00 + msk);

						handle_curr_act_stats(ifd, fpos, &curr, &cnt, &eosaf,
								      rows, act[p]->id, &reset, file_actlst,
								      from_file, &file_magic, rec_hdr_tmp,
								      endian_mismatch, arch_64);
						act[p]->opt_flags = optf;
					}
				}
			}
		}

		if (!cnt) {
			/* Go to next Linux restart, if possible */
			do {
				/* Read next record header */
				eosaf = read_record_hdr(ifd, rec_hdr_tmp, &record_hdr[curr],
							&file_hdr, arch_64, endian_mismatch, UEOF_STOP);
				rtype = record_hdr[curr].record_type;

				if (!eosaf && (rtype != R_RESTART) && (rtype != R_COMMENT)) {
					read_file_stat_bunch(act, curr, ifd, file_hdr.sa_act_nr,
							     file_actlst, endian_mismatch, arch_64,
							     from_file, &file_magic, UEOF_STOP);
				}
				else if (!eosaf && (rtype == R_COMMENT)) {
					/* This was a COMMENT record: print it */
					print_special_record(&record_hdr[curr], flags + S_F_LOCAL_TIME,
							     &tm_start, &tm_end, R_COMMENT, ifd,
							     &rectime, NULL, from_file, 0,
							     &file_magic, &file_hdr, act, &sar_fmt,
							     endian_mismatch, arch_64);
				}
			}
			while (!eosaf && (rtype != R_RESTART));
		}

		/* The last record we read was a RESTART one: Print it */
		if (!eosaf && (record_hdr[curr].record_type == R_RESTART)) {
			print_special_record(&record_hdr[curr], flags + S_F_LOCAL_TIME,
					     &tm_start, &tm_end, R_RESTART, ifd,
					     &rectime, NULL, from_file, 0,
					     &file_magic, &file_hdr, act, &sar_fmt,
					     endian_mismatch, arch_64);
		}
	}
	while (!eosaf);

	close(ifd);

	free(file_actlst);
}