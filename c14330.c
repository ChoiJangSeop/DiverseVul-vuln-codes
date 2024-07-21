bool decompress_file(rzip_control *control)
{
	char *tmp, *tmpoutfile, *infilecopy = NULL;
	int fd_in, fd_out = -1, fd_hist = -1;
	i64 expected_size = 0, free_space;
	struct statvfs fbuf;

	if (!STDIN && !IS_FROM_FILE) {
		struct stat fdin_stat;

		stat(control->infile, &fdin_stat);
		if (!S_ISREG(fdin_stat.st_mode) && (tmp = strrchr(control->infile, '.')) &&
		    strcmp(tmp,control->suffix)) {
			/* make sure infile has an extension. If not, add it
			  * because manipulations may be made to input filename, set local ptr
			*/
			infilecopy = alloca(strlen(control->infile) + strlen(control->suffix) + 1);
			strcpy(infilecopy, control->infile);
			strcat(infilecopy, control->suffix);
		} else
			infilecopy = strdupa(control->infile);
		/* regardless, infilecopy has the input filename */
	}

	if (!STDOUT && !TEST_ONLY) {
		/* if output name already set, use it */
		if (control->outname) {
			control->outfile = strdup(control->outname);
		} else {
			/* default output name from infilecopy
			 * test if outdir specified. If so, strip path from filename of
			 * infilecopy, then remove suffix.
			*/
			if (control->outdir && (tmp = strrchr(infilecopy, '/')))
				tmpoutfile = strdupa(tmp + 1);
			else
				tmpoutfile = strdupa(infilecopy);

			/* remove suffix to make outfile name */
			if ((tmp = strrchr(tmpoutfile, '.')) && !strcmp(tmp, control->suffix))
				*tmp='\0';

			control->outfile = malloc((control->outdir == NULL? 0: strlen(control->outdir)) + strlen(tmpoutfile) + 1);
			if (unlikely(!control->outfile))
				fatal_return(("Failed to allocate outfile name\n"), false);

			if (control->outdir) {	/* prepend control->outdir */
				strcpy(control->outfile, control->outdir);
				strcat(control->outfile, tmpoutfile);
			} else
				strcpy(control->outfile, tmpoutfile);
		}

		if (!STDOUT)
			print_progress("Output filename is: %s\n", control->outfile);
	}

	if ( IS_FROM_FILE ) {
		fd_in = fileno(control->inFILE);
	}
	else if (STDIN) {
		fd_in = open_tmpinfile(control);
		read_tmpinmagic(control);
		if (ENCRYPT)
			failure_return(("Cannot decompress encrypted file from STDIN\n"), false);
		expected_size = control->st_size;
		if (unlikely(!open_tmpinbuf(control)))
			return false;
	} else {
		fd_in = open(infilecopy, O_RDONLY);
		if (unlikely(fd_in == -1)) {
			fatal_return(("Failed to open %s\n", infilecopy), false);
		}
	}
	control->fd_in = fd_in;

	if (!(TEST_ONLY | STDOUT)) {
		fd_out = open(control->outfile, O_WRONLY | O_CREAT | O_EXCL, 0666);
		if (FORCE_REPLACE && (-1 == fd_out) && (EEXIST == errno)) {
			if (unlikely(unlink(control->outfile)))
				fatal_return(("Failed to unlink an existing file: %s\n", control->outfile), false);
			fd_out = open(control->outfile, O_WRONLY | O_CREAT | O_EXCL, 0666);
		}
		if (unlikely(fd_out == -1)) {
			/* We must ensure we don't delete a file that already
			 * exists just because we tried to create a new one */
			control->flags |= FLAG_KEEP_BROKEN;
			fatal_return(("Failed to create %s\n", control->outfile), false);
		}
		fd_hist = open(control->outfile, O_RDONLY);
		if (unlikely(fd_hist == -1))
			fatal_return(("Failed to open history file %s\n", control->outfile), false);

		/* Can't copy permissions from STDIN */
		if (!STDIN)
			if (unlikely(!preserve_perms(control, fd_in, fd_out)))
				return false;
	} else {
		fd_out = open_tmpoutfile(control);
		if (fd_out == -1) {
			fd_hist = -1;
		} else {
			fd_hist = open(control->outfile, O_RDONLY);
			if (unlikely(fd_hist == -1))
				fatal_return(("Failed to open history file %s\n", control->outfile), false);
			/* Unlink temporary file as soon as possible */
			if (unlikely(unlink(control->outfile)))
				fatal_return(("Failed to unlink tmpfile: %s\n", control->outfile), false);
		}
	}

	if (STDOUT) {
		if (unlikely(!open_tmpoutbuf(control)))
			return false;
	}

	if (!STDIN) {
		if (unlikely(!read_magic(control, fd_in, &expected_size)))
			return false;
		if (unlikely(expected_size < 0))
			fatal_return(("Invalid expected size %lld\n", expected_size), false);
	}

	if (!STDOUT && !TEST_ONLY) {
		/* Check if there's enough free space on the device chosen to fit the
		* decompressed file. */
		if (unlikely(fstatvfs(fd_out, &fbuf)))
			fatal_return(("Failed to fstatvfs in decompress_file\n"), false);
		free_space = (i64)fbuf.f_bsize * (i64)fbuf.f_bavail;
		if (free_space < expected_size) {
			if (FORCE_REPLACE)
				print_err("Warning, inadequate free space detected, but attempting to decompress due to -f option being used.\n");
			else
				failure_return(("Inadequate free space to decompress file, use -f to override.\n"), false);
		}
	}
	control->fd_out = fd_out;
	control->fd_hist = fd_hist;

	if (NO_MD5)
		print_verbose("Not performing MD5 hash check\n");
	if (HAS_MD5)
		print_verbose("MD5 ");
	else
		print_verbose("CRC32 ");
	print_verbose("being used for integrity testing.\n");

	if (ENCRYPT)
		if (unlikely(!get_hash(control, 0)))
			return false;

	print_progress("Decompressing...\n");

	if (unlikely(runzip_fd(control, fd_in, fd_out, fd_hist, expected_size) < 0))
		return false;

	if (STDOUT && !TMP_OUTBUF) {
		if (unlikely(!dump_tmpoutfile(control, fd_out)))
			return false;
	}

	/* if we get here, no fatal_return(( errors during decompression */
	print_progress("\r");
	if (!(STDOUT | TEST_ONLY))
		print_progress("Output filename is: %s: ", control->outfile);
	if (!expected_size)
		expected_size = control->st_size;
	if (!ENCRYPT)
		print_progress("[OK] - %lld bytes                                \n", expected_size);
	else
		print_progress("[OK]                                             \n");

	if (TMP_OUTBUF)
		close_tmpoutbuf(control);

	if (fd_out > 0) {
		if (unlikely(close(fd_hist) || close(fd_out)))
			fatal_return(("Failed to close files\n"), false);
	}

	if (unlikely(!STDIN && !STDOUT && !TEST_ONLY && !preserve_times(control, fd_in)))
		return false;

	if ( ! IS_FROM_FILE ) {
		close(fd_in);
	}

	if (!KEEP_FILES && !STDIN) {
		if (unlikely(unlink(control->infile)))
			fatal_return(("Failed to unlink %s\n", infilecopy), false);
	}

	if (ENCRYPT)
		release_hashes(control);

	dealloc(control->outfile);
	return true;
}