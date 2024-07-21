int read_next_sample(int ifd, int action, int curr, char *file, int *rtype, int tab,
		     struct file_magic *file_magic, struct file_activity *file_actlst,
		     struct tm *rectime, struct tm *loctime, int oneof)
{
	int rc;
	char rec_hdr_tmp[MAX_RECORD_HEADER_SIZE];

	/* Read current record */
	if ((rc = read_record_hdr(ifd, rec_hdr_tmp, &record_hdr[curr], &file_hdr,
			    arch_64, endian_mismatch, oneof)) != 0)
		/* End of sa file */
		return rc;

	*rtype = record_hdr[curr].record_type;

	if (*rtype == R_COMMENT) {
		if (action & IGNORE_COMMENT) {
			/* Ignore COMMENT record */
			if (lseek(ifd, MAX_COMMENT_LEN, SEEK_CUR) < MAX_COMMENT_LEN) {
				perror("lseek");
				if (oneof == UEOF_CONT)
					return 2;
				close(ifd);
				exit(2);
			}
			if (action & SET_TIMESTAMPS) {
				sa_get_record_timestamp_struct(flags, &record_hdr[curr],
							       rectime, loctime);
			}
		}
		else {
			/* Display COMMENT record */
			print_special_record(&record_hdr[curr], flags, &tm_start, &tm_end,
					     *rtype, ifd, rectime, loctime, file, tab,
					     file_magic, &file_hdr, act, fmt[f_position],
					     endian_mismatch, arch_64);
		}
	}
	else if (*rtype == R_RESTART) {
		if (action & IGNORE_RESTART) {
			/*
			 * Ignore RESTART record (don't display it)
			 * but anyway we have to read the CPU number that follows it
			 * (unless we don't want to do it now).
			 */
			if (!(action & DONT_READ_CPU_NR)) {
				file_hdr.sa_cpu_nr = read_nr_value(ifd, file, file_magic,
								   endian_mismatch, arch_64, TRUE);
			}
			if (action & SET_TIMESTAMPS) {
				sa_get_record_timestamp_struct(flags, &record_hdr[curr],
							       rectime, loctime);
			}
		}
		else {
			/* Display RESTART record */
			print_special_record(&record_hdr[curr], flags, &tm_start, &tm_end,
					     *rtype, ifd, rectime, loctime, file, tab,
					     file_magic, &file_hdr, act, fmt[f_position],
					     endian_mismatch, arch_64);
		}
	}
	else {
		/*
		 * OK: Previous record was not a special one.
		 * So read now the extra fields.
		 */
		if (read_file_stat_bunch(act, curr, ifd, file_hdr.sa_act_nr, file_actlst,
					 endian_mismatch, arch_64, file, file_magic, oneof) > 0)
			return 2;
		sa_get_record_timestamp_struct(flags, &record_hdr[curr], rectime, loctime);
	}

	return 0;
}