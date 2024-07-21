int main(int argc, char *argv[])
{
	int opt;
	char *line;

	progname = basename(argv[0]);

#if POSIXLY_CORRECT
	cmd_line_options = POSIXLY_CMD_LINE_OPTIONS;
#else
	if (getenv(POSIXLY_CORRECT_STR))
		posixly_correct = 1;
	if (!posixly_correct)
		cmd_line_options = CMD_LINE_OPTIONS;
	else
		cmd_line_options = POSIXLY_CMD_LINE_OPTIONS;
#endif

	setlocale(LC_CTYPE, "");
	setlocale(LC_MESSAGES, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	/* Align `#effective:' comments to column 40 for tty's */
	if (!posixly_correct && isatty(fileno(stdout)))
		print_options |= TEXT_SMART_INDENT;

	while ((opt = getopt_long(argc, argv, cmd_line_options,
		                 long_options, NULL)) != -1) {
		switch (opt) {
			case 'a':  /* acl only */
				if (posixly_correct)
					goto synopsis;
				opt_print_acl = 1;
				break;

			case 'd':  /* default acl only */
				opt_print_default_acl = 1;
				break;

			case 'c':  /* no comments */
				if (posixly_correct)
					goto synopsis;
				opt_comments = 0;
				break;

			case 'e':  /* all #effective comments */
				if (posixly_correct)
					goto synopsis;
				print_options |= TEXT_ALL_EFFECTIVE;
				break;

			case 'E':  /* no #effective comments */
				if (posixly_correct)
					goto synopsis;
				print_options &= ~(TEXT_SOME_EFFECTIVE |
				                   TEXT_ALL_EFFECTIVE);
				break;

			case 'R':  /* recursive */
				if (posixly_correct)
					goto synopsis;
				walk_flags |= WALK_TREE_RECURSIVE;
				break;

			case 'L':  /* follow all symlinks */
				if (posixly_correct)
					goto synopsis;
				walk_flags |= WALK_TREE_LOGICAL;
				walk_flags &= ~WALK_TREE_PHYSICAL;
				break;

			case 'P':  /* skip all symlinks */
				if (posixly_correct)
					goto synopsis;
				walk_flags |= WALK_TREE_PHYSICAL;
				walk_flags &= ~WALK_TREE_LOGICAL;
				break;

			case 's':  /* skip files with only base entries */
				if (posixly_correct)
					goto synopsis;
				opt_skip_base = 1;
				break;

			case 'p':
				if (posixly_correct)
					goto synopsis;
				opt_strip_leading_slash = 0;
				break;

			case 't':
				if (posixly_correct)
					goto synopsis;
				opt_tabular = 1;
				break;

			case 'n':  /* numeric */
				opt_numeric = 1;
				print_options |= TEXT_NUMERIC_IDS;
				break;

			case 'v':  /* print version */
				printf("%s " VERSION "\n", progname);
				return 0;

			case 'h':  /* help */
				help();
				return 0;

			case ':':  /* option missing */
			case '?':  /* unknown option */
			default:
				goto synopsis;
		}
	}

	if (!(opt_print_acl || opt_print_default_acl)) {
		opt_print_acl = 1;
		if (!posixly_correct)
			opt_print_default_acl = 1;
	}
		
	if ((optind == argc) && !posixly_correct)
		goto synopsis;

	do {
		if (optind == argc ||
		    strcmp(argv[optind], "-") == 0) {
			while ((line = next_line(stdin)) != NULL) {
				if (*line == '\0')
					continue;

				had_errors += walk_tree(line, walk_flags, 0,
							do_print, NULL);
			}
			if (!feof(stdin)) {
				fprintf(stderr, _("%s: Standard input: %s\n"),
				        progname, strerror(errno));
				had_errors++;
			}
		} else
			had_errors += walk_tree(argv[optind], walk_flags, 0,
						do_print, NULL);
		optind++;
	} while (optind < argc);

	return had_errors ? 1 : 0;

synopsis:
	fprintf(stderr, _("Usage: %s [-%s] file ...\n"),
	        progname, cmd_line_options);
	fprintf(stderr, _("Try `%s --help' for more information.\n"),
		progname);
	return 2;
}