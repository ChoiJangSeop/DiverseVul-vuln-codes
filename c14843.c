int main(int argc, char *argv[])
{
	bool lrzcat = false, compat = false, recurse = false;
	bool options_file = false, conf_file_compression_set = false; /* for environment and tracking of compression setting */
	struct timeval start_time, end_time;
	struct sigaction handler;
	double seconds,total_time; // for timers
	bool nice_set = false;
	int c, i;
	int hours,minutes;
	extern int optind;
	char *eptr, *av; /* for environment */
	char *endptr = NULL;

        control = &base_control;

	initialise_control(control);

	av = basename(argv[0]);
	if (!strcmp(av, "lrunzip"))
		control->flags |= FLAG_DECOMPRESS;
	else if (!strcmp(av, "lrzcat")) {
		control->flags |= FLAG_DECOMPRESS | FLAG_STDOUT;
		lrzcat = true;
	} else if (!strcmp(av, "lrz")) {
		/* Called in gzip compatible command line mode */
		control->flags &= ~FLAG_SHOW_PROGRESS;
		control->flags &= ~FLAG_KEEP_FILES;
		compat = true;
		long_options[1].name = "stdout";
		long_options[11].name = "keep";
	}

	/* generate crc table */
	CrcGenerateTable();

	/* Get Preloaded Defaults from lrzip.conf
	 * Look in ., $HOME/.lrzip/, /etc/lrzip.
	 * If LRZIP=NOCONFIG is set, then ignore config
	 * If lrzip.conf sets a compression mode, options_file will be true.
	 * This will allow for a test to permit an override of compression mode.
	 * If there is an override, then all compression settings will be reset
	 * and command line switches will prevail, including for --lzma.
	 */
	eptr = getenv("LRZIP");
	if (eptr == NULL)
		options_file = read_config(control);
	else if (!strstr(eptr,"NOCONFIG"))
		options_file = read_config(control);
	if (options_file && (control->flags & FLAG_NOT_LZMA))		/* if some compression set in lrzip.conf    */
		conf_file_compression_set = true;			/* need this to allow command line override */

	while ((c = getopt_long(argc, argv, compat ? coptions : loptions, long_options, &i)) != -1) {
		switch (c) {
		case 'b':
		case 'g':
		case 'l':
		case 'n':
		case 'z':
			/* If some compression was chosen in lrzip.conf, allow this one time
			 * because conf_file_compression_set will be true
			 */
			if ((control->flags & FLAG_NOT_LZMA) && conf_file_compression_set == false)
				failure("Can only use one of -l, -b, -g, -z or -n\n");
			/* Select Compression Mode */
			control->flags &= ~FLAG_NOT_LZMA; /* must clear all compressions first */
			if (c == 'b')
				control->flags |= FLAG_BZIP2_COMPRESS;
			else if (c == 'g')
				control->flags |= FLAG_ZLIB_COMPRESS;
			else if (c == 'l')
				control->flags |= FLAG_LZO_COMPRESS;
			else if (c == 'n')
				control->flags |= FLAG_NO_COMPRESS;
			else if (c == 'z')
				control->flags |= FLAG_ZPAQ_COMPRESS;
			/* now FLAG_NOT_LZMA will evaluate as true */
			conf_file_compression_set = false;
			break;
		case '/':							/* LZMA Compress selected */
			control->flags &= ~FLAG_NOT_LZMA;			/* clear alternate compression flags */
			break;
		case 'c':
			if (compat) {
				control->flags |= FLAG_KEEP_FILES;
				set_stdout(control);
				break;
			}
			/* FALLTHRU */
		case 'C':
			control->flags |= FLAG_CHECK;
			control->flags |= FLAG_HASH;
			break;
		case 'd':
			control->flags |= FLAG_DECOMPRESS;
			break;
		case 'D':
			control->flags &= ~FLAG_KEEP_FILES;
			break;
		case 'e':
			control->flags |= FLAG_ENCRYPT;
			control->passphrase = optarg;
			break;
		case 'f':
			control->flags |= FLAG_FORCE_REPLACE;
			break;
		case 'h':
			usage(compat);
			exit(0);
			break;
		case 'H':
			control->flags |= FLAG_HASH;
			break;
		case 'i':
			control->flags |= FLAG_INFO;
			control->flags &= ~FLAG_DECOMPRESS;
			break;
		case 'k':
			if (compat) {
				control->flags |= FLAG_KEEP_FILES;
				break;
			}
			/* FALLTHRU */
		case 'K':
			control->flags |= FLAG_KEEP_BROKEN;
			break;
		case 'L':
			if (compat) {
				license();
				exit(0);
			}
			control->compression_level = strtol(optarg, &endptr, 10);
			if (control->compression_level < 1 || control->compression_level > 9)
				failure("Invalid compression level (must be 1-9)\n");
			if (*endptr)
				failure("Extra characters after compression level: \'%s\'\n", endptr);
			break;
		case 'm':
			control->ramsize = strtol(optarg, &endptr, 10) * 1024 * 1024 * 100;
			if (*endptr)
				failure("Extra characters after ramsize: \'%s\'\n", endptr);
			break;
		case 'N':
			nice_set = true;
			control->nice_val = strtol(optarg, &endptr, 10);
			if (control->nice_val < PRIO_MIN || control->nice_val > PRIO_MAX)
				failure("Invalid nice value (must be %d...%d)\n", PRIO_MIN, PRIO_MAX);
			if (*endptr)
				failure("Extra characters after nice level: \'%s\'\n", endptr);
			break;
		case 'o':
			if (control->outdir)
				failure("Cannot have -o and -O together\n");
			if (unlikely(STDOUT))
				failure("Cannot specify an output filename when outputting to stdout\n");
			control->outname = optarg;
			control->suffix = "";
			break;
		case 'O':
			if (control->outname)	/* can't mix -o and -O */
				failure("Cannot have options -o and -O together\n");
			if (unlikely(STDOUT))
				failure("Cannot specify an output directory when outputting to stdout\n");
			control->outdir = malloc(strlen(optarg) + 2);
			if (control->outdir == NULL)
				fatal("Failed to allocate for outdir\n");
			strcpy(control->outdir,optarg);
			if (strcmp(optarg+strlen(optarg) - 1, "/")) 	/* need a trailing slash */
				strcat(control->outdir, "/");
			break;
		case 'p':
			control->threads = strtol(optarg, &endptr, 10);
			if (control->threads < 1)
				failure("Must have at least one thread\n");
			if (*endptr)
				failure("Extra characters after number of threads: \'%s\'\n", endptr);
			break;
		case 'P':
			control->flags |= FLAG_SHOW_PROGRESS;
			break;
		case 'q':
			control->flags &= ~FLAG_SHOW_PROGRESS;
			break;
		case 'r':
			recurse = true;
			break;
		case 'S':
			if (control->outname)
				failure("Specified output filename already, can't specify an extension.\n");
			if (unlikely(STDOUT))
				failure("Cannot specify a filename suffix when outputting to stdout\n");
			control->suffix = optarg;
			break;
		case 't':
			if (control->outname)
				failure("Cannot specify an output file name when just testing.\n");
			if (compat)
				control->flags |= FLAG_KEEP_FILES;
			if (!KEEP_FILES)
				failure("Doubt that you want to delete a file when just testing.\n");
			control->flags |= FLAG_TEST_ONLY;
			break;
		case 'T':
			control->flags &= ~FLAG_THRESHOLD;
			break;
		case 'U':
			control->flags |= FLAG_UNLIMITED;
			break;
		case 'v':
			/* set verbosity flag */
			if (!(control->flags & FLAG_SHOW_PROGRESS))
				control->flags |= FLAG_SHOW_PROGRESS;
			else if (!(control->flags & FLAG_VERBOSITY) && !(control->flags & FLAG_VERBOSITY_MAX))
				control->flags |= FLAG_VERBOSITY;
			else if ((control->flags & FLAG_VERBOSITY)) {
				control->flags &= ~FLAG_VERBOSITY;
				control->flags |= FLAG_VERBOSITY_MAX;
			}
			break;
		case 'V':
			control->msgout = stdout;
			print_output("lrzip version %s\n", PACKAGE_VERSION);
			exit(0);
			break;
		case 'w':
			control->window = strtol(optarg, &endptr, 10);
			if (control->window < 1)
				failure("Window must be positive\n");
			if (*endptr)
				failure("Extra characters after window size: \'%s\'\n", endptr);
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			control->compression_level = c - '0';
			break;
		default:
			usage(compat);
			return 2;
		}
	}

	argc -= optind;
	argv += optind;

	if (control->outname) {
		if (argc > 1)
			failure("Cannot specify output filename with more than 1 file\n");
		if (recurse)
			failure("Cannot specify output filename with recursive\n");
	}

	if (VERBOSE && !SHOW_PROGRESS) {
		print_err("Cannot have -v and -q options. -v wins.\n");
		control->flags |= FLAG_SHOW_PROGRESS;
	}

	if (UNLIMITED && control->window) {
		print_err("If -U used, cannot specify a window size with -w.\n");
		control->window = 0;
	}

	if (argc < 1)
		control->flags |= FLAG_STDIN;

	if (UNLIMITED && STDIN) {
		print_err("Cannot have -U and stdin, unlimited mode disabled.\n");
		control->flags &= ~FLAG_UNLIMITED;
	}

	setup_overhead(control);

	/* Set the main nice value to half that of the backend threads since
	 * the rzip stage is usually the rate limiting step */
	control->current_priority = getpriority(PRIO_PROCESS, 0);
	if (nice_set) {
		if (!NO_COMPRESS) {
			/* If niceness can't be set. just reset process priority */
			if (unlikely(setpriority(PRIO_PROCESS, 0, control->nice_val/2) == -1)) {
				print_err("Warning, unable to set nice value %d...Resetting to %d\n",
					control->nice_val, control->current_priority);
				setpriority(PRIO_PROCESS, 0, (control->nice_val=control->current_priority));
			}
		} else {
			if (unlikely(setpriority(PRIO_PROCESS, 0, control->nice_val) == -1)) {
				print_err("Warning, unable to set nice value %d...Resetting to %d\n",
					control->nice_val, control->current_priority);
				setpriority(PRIO_PROCESS, 0, (control->nice_val=control->current_priority));
			}
		}
	}

	/* One extra iteration for the case of no parameters means we will default to stdin/out */
	for (i = 0; i <= argc; i++) {
		char *dirlist = NULL, *infile = NULL;
		int direntries = 0, curentry = 0;

		if (i < argc)
			infile = argv[i];
		else if (!(i == 0 && STDIN))
			break;
		if (infile) {
			if ((strcmp(infile, "-") == 0))
				control->flags |= FLAG_STDIN;
			else {
				bool isdir = false;
				struct stat istat;

				if (unlikely(stat(infile, &istat)))
					failure("Failed to stat %s\n", infile);
				isdir = S_ISDIR(istat.st_mode);
				if (!recurse && (isdir || !S_ISREG(istat.st_mode))) {
					failure("lrzip only works directly on regular FILES.\n"
					"Use -r recursive, lrztar or pipe through tar for compressing directories.\n");
				}
				if (recurse && !isdir)
					failure("%s not a directory, -r recursive needs a directory\n", infile);
			}
		}

		if (recurse) {
			if (unlikely(STDIN || STDOUT))
				failure("Cannot use -r recursive with STDIO\n");
			recurse_dirlist(infile, &dirlist, &direntries);
		}

		if (INFO && STDIN)
			failure("Will not get file info from STDIN\n");
recursion:
		if (recurse) {
			if (curentry >= direntries) {
				infile = NULL;
				continue;
			}
			infile = dirlist + MAX_PATH_LEN * curentry++;
		}
		control->infile = infile;

		/* If no output filename is specified, and we're using
		 * stdin, use stdout */
		if ((control->outname && (strcmp(control->outname, "-") == 0)) ||
		    (!control->outname && STDIN) || lrzcat)
				set_stdout(control);

		if (lrzcat) {
			control->msgout = stderr;
			control->outFILE = stdout;
			register_outputfile(control, control->msgout);
		}

		if (!STDOUT) {
			control->msgout = stdout;
			register_outputfile(control, control->msgout);
		}

		if (STDIN)
			control->inFILE = stdin;

		/* Implement signal handler only once flags are set */
		sigemptyset(&handler.sa_mask);
		handler.sa_flags = 0;
		handler.sa_handler = &sighandler;
		sigaction(SIGTERM, &handler, 0);
		sigaction(SIGINT, &handler, 0);

		if (!FORCE_REPLACE) {
			if (STDIN && isatty(fileno((FILE *)stdin))) {
				print_err("Will not read stdin from a terminal. Use -f to override.\n");
				usage(compat);
				exit (1);
			}
			if (!TEST_ONLY && STDOUT && isatty(fileno((FILE *)stdout)) && !compat) {
				print_err("Will not write stdout to a terminal. Use -f to override.\n");
				usage(compat);
				exit (1);
			}
		}

		if (CHECK_FILE) {
			if (!DECOMPRESS) {
				print_err("Can only check file written on decompression.\n");
				control->flags &= ~FLAG_CHECK;
			} else if (STDOUT) {
				print_err("Can't check file written when writing to stdout. Checking disabled.\n");
				control->flags &= ~FLAG_CHECK;
			}
		}

		setup_ram(control);
		show_summary();

		gettimeofday(&start_time, NULL);

		if (unlikely((STDIN || STDOUT) && ENCRYPT))
			failure("Unable to work from STDIO while reading password\n");

		memcpy(&local_control, &base_control, sizeof(rzip_control));
		if (DECOMPRESS || TEST_ONLY)
			decompress_file(&local_control);
		else if (INFO)
			get_fileinfo(&local_control);
		else
			compress_file(&local_control);

		/* compute total time */
		gettimeofday(&end_time, NULL);
		total_time = (end_time.tv_sec + (double)end_time.tv_usec / 1000000) -
			      (start_time.tv_sec + (double)start_time.tv_usec / 1000000);
		hours = (int)total_time / 3600;
		minutes = (int)(total_time / 60) % 60;
		seconds = total_time - hours * 3600 - minutes * 60;
		if (!INFO)
			print_progress("Total time: %02d:%02d:%05.2f\n", hours, minutes, seconds);
		if (recurse)
			goto recursion;
	}

	return 0;
}