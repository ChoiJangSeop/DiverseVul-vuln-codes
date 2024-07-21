int main(int argc, char *argv[])
{
	struct libmnt_table *tb = NULL;
	char **tabfiles = NULL;
	int direction = MNT_ITER_FORWARD;
	int verify = 0;
	int c, rc = -1, timeout = -1;
	int ntabfiles = 0, tabtype = 0;
	char *outarg = NULL;
	size_t i;
	int force_tree = 0, istree = 0;

	struct libscols_table *table = NULL;

	enum {
		FINDMNT_OPT_VERBOSE = CHAR_MAX + 1,
		FINDMNT_OPT_TREE,
		FINDMNT_OPT_OUTPUT_ALL,
		FINDMNT_OPT_PSEUDO,
		FINDMNT_OPT_REAL,
		FINDMNT_OPT_VFS_ALL,
		FINDMNT_OPT_SHADOWED,
		FINDMNT_OPT_DELETED,
	};

	static const struct option longopts[] = {
		{ "all",	    no_argument,       NULL, 'A'		 },
		{ "ascii",	    no_argument,       NULL, 'a'		 },
		{ "bytes",	    no_argument,       NULL, 'b'		 },
		{ "canonicalize",   no_argument,       NULL, 'c'		 },
		{ "deleted",        no_argument,       NULL, FINDMNT_OPT_DELETED },
		{ "direction",	    required_argument, NULL, 'd'		 },
		{ "df",		    no_argument,       NULL, 'D'		 },
		{ "evaluate",	    no_argument,       NULL, 'e'		 },
		{ "first-only",	    no_argument,       NULL, 'f'		 },
		{ "fstab",	    no_argument,       NULL, 's'		 },
		{ "help",	    no_argument,       NULL, 'h'		 },
		{ "invert",	    no_argument,       NULL, 'i'		 },
		{ "json",	    no_argument,       NULL, 'J'		 },
		{ "kernel",	    no_argument,       NULL, 'k'		 },
		{ "list",	    no_argument,       NULL, 'l'		 },
		{ "mountpoint",	    required_argument, NULL, 'M'		 },
		{ "mtab",	    no_argument,       NULL, 'm'		 },
		{ "noheadings",	    no_argument,       NULL, 'n'		 },
		{ "notruncate",	    no_argument,       NULL, 'u'		 },
		{ "options",	    required_argument, NULL, 'O'		 },
		{ "output",	    required_argument, NULL, 'o'		 },
		{ "output-all",	    no_argument,       NULL, FINDMNT_OPT_OUTPUT_ALL },
		{ "poll",	    optional_argument, NULL, 'p'		 },
		{ "pairs",	    no_argument,       NULL, 'P'		 },
		{ "raw",	    no_argument,       NULL, 'r'		 },
		{ "types",	    required_argument, NULL, 't'		 },
		{ "nocanonicalize", no_argument,       NULL, 'C'		 },
		{ "nofsroot",	    no_argument,       NULL, 'v'		 },
		{ "submounts",	    no_argument,       NULL, 'R'		 },
		{ "source",	    required_argument, NULL, 'S'		 },
		{ "tab-file",	    required_argument, NULL, 'F'		 },
		{ "task",	    required_argument, NULL, 'N'		 },
		{ "target",	    required_argument, NULL, 'T'		 },
		{ "timeout",	    required_argument, NULL, 'w'		 },
		{ "uniq",	    no_argument,       NULL, 'U'		 },
		{ "verify",	    no_argument,       NULL, 'x'		 },
		{ "version",	    no_argument,       NULL, 'V'		 },
		{ "verbose",	    no_argument,       NULL, FINDMNT_OPT_VERBOSE },
		{ "tree",	    no_argument,       NULL, FINDMNT_OPT_TREE	 },
		{ "real",	    no_argument,       NULL, FINDMNT_OPT_REAL	 },
		{ "pseudo",	    no_argument,       NULL, FINDMNT_OPT_PSEUDO	 },
		{ "vfs-all",	    no_argument,       NULL, FINDMNT_OPT_VFS_ALL },
		{ "shadowed",       no_argument,       NULL, FINDMNT_OPT_SHADOWED },
		{ NULL, 0, NULL, 0 }
	};

	static const ul_excl_t excl[] = {	/* rows and cols in ASCII order */
		{ 'C', 'c'},			/* [no]canonicalize */
		{ 'C', 'e' },			/* nocanonicalize, evaluate */
		{ 'J', 'P', 'r','x' },		/* json,pairs,raw,verify */
		{ 'M', 'T' },			/* mountpoint, target */
		{ 'N','k','m','s' },		/* task,kernel,mtab,fstab */
		{ 'P','l','r','x' },		/* pairs,list,raw,verify */
		{ 'p','x' },			/* poll,verify */
		{ 'm','p','s' },		/* mtab,poll,fstab */
		{ FINDMNT_OPT_PSEUDO, FINDMNT_OPT_REAL },
		{ 0 }
	};
	int excl_st[ARRAY_SIZE(excl)] = UL_EXCL_STATUS_INIT;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	close_stdout_atexit();

	/* default output format */
	flags |= FL_TREE;

	while ((c = getopt_long(argc, argv,
				"AabCcDd:ehiJfF:o:O:p::PklmM:nN:rst:uvRS:T:Uw:Vx",
				longopts, NULL)) != -1) {

		err_exclusive_options(c, longopts, excl, excl_st);

		switch(c) {
		case 'A':
			flags |= FL_ALL;
			break;
		case 'a':
			flags |= FL_ASCII;
			break;
		case 'b':
			flags |= FL_BYTES;
			break;
		case 'C':
			flags |= FL_NOCACHE;
			break;
		case 'c':
			flags |= FL_CANONICALIZE;
			break;
		case 'D':
			flags &= ~FL_TREE;
			flags |= FL_DF;
			break;
		case 'd':
			if (!strcmp(optarg, "forward"))
				direction = MNT_ITER_FORWARD;
			else if (!strcmp(optarg, "backward"))
				direction = MNT_ITER_BACKWARD;
			else
				errx(EXIT_FAILURE,
					_("unknown direction '%s'"), optarg);
			break;
		case 'e':
			flags |= FL_EVALUATE;
			break;
		case 'i':
			flags |= FL_INVERT;
			break;
		case 'J':
			flags |= FL_JSON;
			break;
		case 'f':
			flags |= FL_FIRSTONLY;
			break;
		case 'F':
			tabfiles = append_tabfile(tabfiles, &ntabfiles, optarg);
			break;
		case 'u':
			disable_columns_truncate();
			break;
		case 'o':
			outarg = optarg;
			break;
		case FINDMNT_OPT_OUTPUT_ALL:
			for (ncolumns = 0; ncolumns < ARRAY_SIZE(infos); ncolumns++) {
				if (is_tabdiff_column(ncolumns))
					continue;
				columns[ncolumns] = ncolumns;
			}
			break;
		case 'O':
			set_match(COL_OPTIONS, optarg);
			break;
		case 'p':
			if (optarg) {
				nactions = string_to_idarray(optarg,
						actions, ARRAY_SIZE(actions),
						poll_action_name_to_id);
				if (nactions < 0)
					exit(EXIT_FAILURE);
			}
			flags |= FL_POLL;
			flags &= ~FL_TREE;
			break;
		case 'P':
			flags |= FL_EXPORT;
			flags &= ~FL_TREE;
			break;
		case 'm':		/* mtab */
			tabtype = TABTYPE_MTAB;
			flags &= ~FL_TREE;
			break;
		case 's':		/* fstab */
			tabtype = TABTYPE_FSTAB;
			flags &= ~FL_TREE;
			break;
		case 'k':		/* kernel (mountinfo) */
			tabtype = TABTYPE_KERNEL;
			break;
		case 't':
			set_match(COL_FSTYPE, optarg);
			break;
		case 'r':
			flags &= ~FL_TREE;	/* disable the default */
			flags |= FL_RAW;	/* enable raw */
			break;
		case 'l':
			flags &= ~FL_TREE;	/* disable the default */
			break;
		case 'n':
			flags |= FL_NOHEADINGS;
			break;
		case 'N':
			tabtype = TABTYPE_KERNEL;
			tabfiles = append_pid_tabfile(tabfiles, &ntabfiles,
					strtou32_or_err(optarg,
						_("invalid TID argument")));
			break;
		case 'v':
			flags |= FL_NOFSROOT;
			break;
		case 'R':
			flags |= FL_SUBMOUNTS;
			break;
		case 'S':
			set_source_match(optarg);
			flags |= FL_NOSWAPMATCH;
			break;
		case 'M':
			flags |= FL_STRICTTARGET;
			/* fallthrough */
		case 'T':
			set_match(COL_TARGET, optarg);
			flags |= FL_NOSWAPMATCH;
			break;
		case 'U':
			flags |= FL_UNIQ;
			break;
		case 'w':
			timeout = strtos32_or_err(optarg, _("invalid timeout argument"));
			break;
		case 'x':
			verify = 1;
			break;
		case FINDMNT_OPT_VERBOSE:
			flags |= FL_VERBOSE;
			break;
		case FINDMNT_OPT_TREE:
			force_tree = 1;
			break;
		case FINDMNT_OPT_PSEUDO:
			flags |= FL_PSEUDO;
			break;
		case FINDMNT_OPT_REAL:
			flags |= FL_REAL;
			break;
		case FINDMNT_OPT_VFS_ALL:
			flags |= FL_VFS_ALL;
			break;
		case FINDMNT_OPT_SHADOWED:
			flags |= FL_SHADOWED;
			break;
		case FINDMNT_OPT_DELETED:
			flags |= FL_DELETED;
			break;
		case 'h':
			usage();
		case 'V':
			print_version(EXIT_SUCCESS);
		default:
			errtryhelp(EXIT_FAILURE);
		}
	}

	if (!ncolumns && (flags & FL_DF)) {
		add_column(columns, ncolumns++, COL_SOURCE);
		add_column(columns, ncolumns++, COL_FSTYPE);
		add_column(columns, ncolumns++, COL_SIZE);
		add_column(columns, ncolumns++, COL_USED);
		add_column(columns, ncolumns++, COL_AVAIL);
		add_column(columns, ncolumns++, COL_USEPERC);
		add_column(columns, ncolumns++, COL_TARGET);
	}

	/* default columns */
	if (!ncolumns) {
		if (flags & FL_POLL)
			add_column(columns, ncolumns++, COL_ACTION);

		add_column(columns, ncolumns++, COL_TARGET);
		add_column(columns, ncolumns++, COL_SOURCE);
		add_column(columns, ncolumns++, COL_FSTYPE);
		add_column(columns, ncolumns++, COL_OPTIONS);
	}

	if (outarg && string_add_to_idarray(outarg, columns, ARRAY_SIZE(columns),
					 &ncolumns, column_name_to_id) < 0)
		return EXIT_FAILURE;

	if (!tabtype)
		tabtype = verify ? TABTYPE_FSTAB : TABTYPE_KERNEL;

	if ((flags & FL_POLL) && ntabfiles > 1)
		errx(EXIT_FAILURE, _("--poll accepts only one file, but more specified by --tab-file"));

	if (optind < argc && (get_match(COL_SOURCE) || get_match(COL_TARGET)))
		errx(EXIT_FAILURE, _(
			"options --target and --source can't be used together "
			"with command line element that is not an option"));

	if (optind < argc)
		set_source_match(argv[optind++]);	/* dev/tag/mountpoint/maj:min */
	if (optind < argc)
		set_match(COL_TARGET, argv[optind++]);	/* mountpoint */

	if ((flags & FL_SUBMOUNTS) && is_listall_mode())
		/* don't care about submounts if list all mounts */
		flags &= ~FL_SUBMOUNTS;

	if (!(flags & FL_SUBMOUNTS) && ((flags & FL_FIRSTONLY)
	    || get_match(COL_TARGET)
	    || get_match(COL_SOURCE)
	    || get_match(COL_MAJMIN)))
		flags &= ~FL_TREE;

	if (!(flags & FL_NOSWAPMATCH) &&
	    !get_match(COL_TARGET) && get_match(COL_SOURCE)) {
		/*
		 * Check if we can swap source and target, it's
		 * not possible if the source is LABEL=/UUID=
		 */
		const char *x = get_match(COL_SOURCE);

		if (!strncmp(x, "LABEL=", 6) || !strncmp(x, "UUID=", 5) ||
		    !strncmp(x, "PARTLABEL=", 10) || !strncmp(x, "PARTUUID=", 9))
			flags |= FL_NOSWAPMATCH;
	}

	/*
	 * initialize libmount
	 */
	mnt_init_debug(0);

	tb = parse_tabfiles(tabfiles, ntabfiles, tabtype);
	if (!tb)
		goto leave;

	if (tabtype == TABTYPE_MTAB && tab_is_kernel(tb))
		tabtype = TABTYPE_KERNEL;

	istree = tab_is_tree(tb);
	if (istree && force_tree)
		flags |= FL_TREE;

	if ((flags & FL_TREE) && (ntabfiles > 1 || !istree))
		flags &= ~FL_TREE;

	if (!(flags & FL_NOCACHE)) {
		cache = mnt_new_cache();
		if (!cache) {
			warn(_("failed to initialize libmount cache"));
			goto leave;
		}
		mnt_table_set_cache(tb, cache);

		if (tabtype != TABTYPE_KERNEL)
			cache_set_targets(cache);
	}

	if (flags & FL_UNIQ)
		mnt_table_uniq_fs(tb, MNT_UNIQ_KEEPTREE, uniq_fs_target_cmp);

	if (verify) {
		rc = verify_table(tb);
		goto leave;
	}

	/*
	 * initialize libsmartcols
	 */
	scols_init_debug(0);
	table = scols_new_table();
	if (!table) {
		warn(_("failed to allocate output table"));
		goto leave;
	}
	scols_table_enable_raw(table,        !!(flags & FL_RAW));
	scols_table_enable_export(table,     !!(flags & FL_EXPORT));
	scols_table_enable_json(table,       !!(flags & FL_JSON));
	scols_table_enable_ascii(table,      !!(flags & FL_ASCII));
	scols_table_enable_noheadings(table, !!(flags & FL_NOHEADINGS));

	if (flags & FL_JSON)
		scols_table_set_name(table, "filesystems");

	for (i = 0; i < ncolumns; i++) {
		struct libscols_column *cl;
		int fl = get_column_flags(i);
		int id = get_column_id(i);

		if (!(flags & FL_TREE))
			fl &= ~SCOLS_FL_TREE;

		if (!(flags & FL_POLL) && is_tabdiff_column(id)) {
			warnx(_("%s column is requested, but --poll "
			       "is not enabled"), get_column_name(i));
			goto leave;
		}
		cl = scols_table_new_column(table, get_column_name(i),
					get_column_whint(i), fl);
		if (!cl)	{
			warn(_("failed to allocate output column"));
			goto leave;
		}
		/* multi-line cells (now used for SOURCES) */
		if (fl & SCOLS_FL_WRAP) {
			scols_column_set_wrapfunc(cl,
						scols_wrapnl_chunksize,
						scols_wrapnl_nextchunk,
						NULL);
			scols_column_set_safechars(cl, "\n");
		}
		if (flags & FL_JSON) {
			switch (id) {
			case COL_SIZE:
			case COL_AVAIL:
			case COL_USED:
				if (!(flags & FL_BYTES))
					break;
				/* fallthrough */
			case COL_ID:
			case COL_PARENT:
			case COL_FREQ:
			case COL_PASSNO:
			case COL_TID:
				scols_column_set_json_type(cl, SCOLS_JSON_NUMBER);
				break;
			case COL_DELETED:
				scols_column_set_json_type(cl, SCOLS_JSON_BOOLEAN);
				break;
			default:
				if (fl & SCOLS_FL_WRAP)
					scols_column_set_json_type(cl, SCOLS_JSON_ARRAY_STRING);
				else
					scols_column_set_json_type(cl, SCOLS_JSON_STRING);
				break;
			}
		}
	}

	/*
	 * Fill in data to the output table
	 */
	if (flags & FL_POLL) {
		/* poll mode (accept the first tabfile only) */
		rc = poll_table(tb, tabfiles ? *tabfiles : _PATH_PROC_MOUNTINFO, timeout, table, direction);

	} else if ((flags & FL_TREE) && !(flags & FL_SUBMOUNTS)) {
		/* whole tree */
		rc = create_treenode(table, tb, NULL, NULL);
	} else {
		/* whole list of sub-tree */
		rc = add_matching_lines(tb, table, direction);

		if (rc != 0
		    && tabtype == TABTYPE_KERNEL
		    && (flags & FL_NOSWAPMATCH)
		    && !(flags & FL_STRICTTARGET)
		    && get_match(COL_TARGET)) {
			/*
			 * Found nothing, maybe the --target is regular file,
			 * try it again with extra functionality for target
			 * match
			 */
			enable_extra_target_match(tb);
			rc = add_matching_lines(tb, table, direction);
		}
	}

	/*
	 * Print the output table for non-poll modes
	 */
	if (!rc && !(flags & FL_POLL))
		scols_print_table(table);
leave:
	scols_unref_table(table);

	mnt_unref_table(tb);
	mnt_unref_cache(cache);

	free(tabfiles);
#ifdef HAVE_LIBUDEV
	udev_unref(udev);
#endif
	return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}