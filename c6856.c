void handle_curr_act_stats(int ifd, off_t fpos, int *curr, long *cnt, int *eosaf,
			   int rows, unsigned int act_id, int *reset,
			   struct file_activity *file_actlst, char *file,
			   struct file_magic *file_magic, void *rec_hdr_tmp,
			   int endian_mismatch, int arch_64)
{
	int p, reset_cd;
	unsigned long lines = 0;
	unsigned char rtype;
	int davg = 0, next, inc = 0;

	if (lseek(ifd, fpos, SEEK_SET) < fpos) {
		perror("lseek");
		exit(2);
	}

	/*
	 * Restore the first stats collected.
	 * Used to compute the rate displayed on the first line.
	 */
	copy_structures(act, id_seq, record_hdr, !*curr, 2);

	*cnt  = count;

	/* Assess number of lines printed when a bitmap is used */
	p = get_activity_position(act, act_id, EXIT_IF_NOT_FOUND);
	if (act[p]->bitmap) {
		inc = count_bits(act[p]->bitmap->b_array,
				 BITMAP_SIZE(act[p]->bitmap->b_size));
	}
	reset_cd = 1;

	do {
		/*
		 * Display <count> lines of stats.
		 * Start with reading current sample's record header.
		 */
		*eosaf = read_record_hdr(ifd, rec_hdr_tmp, &record_hdr[*curr],
					 &file_hdr, arch_64, endian_mismatch, UEOF_STOP);
		rtype = record_hdr[*curr].record_type;

		if (!*eosaf && (rtype != R_RESTART) && (rtype != R_COMMENT)) {
			/* Read the extra fields since it's not a special record */
			read_file_stat_bunch(act, *curr, ifd, file_hdr.sa_act_nr, file_actlst,
					     endian_mismatch, arch_64, file, file_magic, UEOF_STOP);
		}

		if ((lines >= rows) || !lines) {
			lines = 0;
			dis = 1;
		}
		else
			dis = 0;

		if (!*eosaf && (rtype != R_RESTART)) {

			if (rtype == R_COMMENT) {
				/* Display comment */
				next = print_special_record(&record_hdr[*curr], flags + S_F_LOCAL_TIME,
							    &tm_start, &tm_end, R_COMMENT, ifd,
							    &rectime, NULL, file, 0,
							    file_magic, &file_hdr, act, &sar_fmt,
							    endian_mismatch, arch_64);
				if (next && lines) {
					/*
					 * A line of comment was actually displayed: Count it in the
					 * total number of displayed lines.
					 * If no lines of stats had been previously displayed, ignore it
					 * to make sure the header line will be displayed.
					 */
					lines++;
				}
				continue;
			}

			/* next is set to 1 when we were close enough to desired interval */
			next = write_stats(*curr, USE_SA_FILE, cnt, tm_start.use, tm_end.use,
					   *reset, act_id, reset_cd);
			reset_cd = 0;
			if (next && (*cnt > 0)) {
				(*cnt)--;
			}

			if (next) {
				davg++;
				*curr ^= 1;

				if (inc) {
					lines += inc;
				}
				else {
					lines += act[p]->nr[*curr];
				}
			}
			*reset = FALSE;
		}
	}
	while (*cnt && !*eosaf && (rtype != R_RESTART));

	if (davg) {
		write_stats_avg(!*curr, USE_SA_FILE, act_id);
	}

	*reset = TRUE;
}