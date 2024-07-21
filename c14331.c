static i64 runzip_chunk(rzip_control *control, int fd_in, i64 expected_size, i64 tally)
{
	uint32 good_cksum, cksum = 0;
	i64 len, ofs, total = 0;
	int l = -1, p = 0;
	char chunk_bytes;
	struct stat st;
	uchar head;
	void *ss;
	bool err = false;

	/* for display of progress */
	unsigned long divisor[] = {1,1024,1048576,1073741824U};
	char *suffix[] = {"","KB","MB","GB"};
	double prog_done, prog_tsize;
	int divisor_index;

	if (expected_size > (i64)10737418240ULL)	/* > 10GB */
		divisor_index = 3;
	else if (expected_size > 10485760)	/* > 10MB */
		divisor_index = 2;
	else if (expected_size > 10240)	/* > 10KB */
		divisor_index = 1;
	else
		divisor_index = 0;

	prog_tsize = (long double)expected_size / (long double)divisor[divisor_index];

	/* Determine the chunk_byte width size. Versions < 0.4 used 4
	 * bytes for all offsets, version 0.4 used 8 bytes. Versions 0.5+ use
	 * a variable number of bytes depending on chunk size.*/
	if (control->major_version == 0 && control->minor_version < 4)
		chunk_bytes = 4;
	else if (control->major_version == 0 && control->minor_version == 4)
		chunk_bytes = 8;
	else {
		print_maxverbose("Reading chunk_bytes at %lld\n", get_readseek(control, fd_in));
		/* Read in the stored chunk byte width from the file */
		if (unlikely(read_1g(control, fd_in, &chunk_bytes, 1) != 1))
			fatal_return(("Failed to read chunk_bytes size in runzip_chunk\n"), -1);
		if (unlikely(chunk_bytes < 1 || chunk_bytes > 8))
			failure_return(("chunk_bytes %d is invalid in runzip_chunk\n", chunk_bytes), -1);
	}
	if (!tally && expected_size)
		print_maxverbose("Expected size: %lld\n", expected_size);
	print_maxverbose("Chunk byte width: %d\n", chunk_bytes);

	ofs = seekcur_fdin(control);
	if (unlikely(ofs == -1))
		fatal_return(("Failed to seek input file in runzip_fd\n"), -1);

	if (fstat(fd_in, &st) || st.st_size - ofs == 0)
		return 0;

	ss = open_stream_in(control, fd_in, NUM_STREAMS, chunk_bytes);
	if (unlikely(!ss))
		failure_return(("Failed to open_stream_in in runzip_chunk\n"), -1);

	/* All chunks were unnecessarily encoded 8 bytes wide version 0.4x */
	if (control->major_version == 0 && control->minor_version == 4)
		control->chunk_bytes = 8;
	else
		control->chunk_bytes = 2;

	while ((len = read_header(control, ss, &head)) || head) {
		i64 u;
		if (unlikely(len == -1))
			return -1;
		switch (head) {
			case 0:
				u = unzip_literal(control, ss, len, &cksum);
				if (unlikely(u == -1)) {
					close_stream_in(control, ss);
					return -1;
				}
				total += u;
				break;

			default:
				u = unzip_match(control, ss, len, &cksum, chunk_bytes);
				if (unlikely(u == -1)) {
					close_stream_in(control, ss);
					return -1;
				}
				total += u;
				break;
		}
		if (expected_size) {
			p = 100 * ((double)(tally + total) / (double)expected_size);
			if (p / 10 != l / 10)  {
				prog_done = (double)(tally + total) / (double)divisor[divisor_index];
				print_progress("%3d%%  %9.2f / %9.2f %s\r",
						p, prog_done, prog_tsize, suffix[divisor_index] );
				l = p;
			}
		}
	}

	if (!HAS_MD5) {
		good_cksum = read_u32(control, ss, 0, &err);
		if (unlikely(err)) {
			close_stream_in(control, ss);
			return -1;
		}
		if (unlikely(good_cksum != cksum)) {
			close_stream_in(control, ss);
			failure_return(("Bad checksum: 0x%08x - expected: 0x%08x\n", cksum, good_cksum), -1);
		}
		print_maxverbose("Checksum for block: 0x%08x\n", cksum);
	}

	if (unlikely(close_stream_in(control, ss)))
		fatal("Failed to close stream!\n");

	/* We can now safely delete sinfo and pthread data of all threads
	 * created. */
	clear_rulist(control);

	return total;
}