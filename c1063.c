main (int argc, char **argv)
{
  char *keys;
  size_t keycc, oldcc, keyalloc;
  int with_filenames;
  int opt, cc, status, prepended;
  int prev_optind, last_recursive;
  int default_context;
  FILE *fp;

  exit_failure = EXIT_TROUBLE;
  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  program_name = argv[0];

  keys = NULL;
  keycc = 0;
  with_filenames = 0;
  eolbyte = '\n';
  filename_mask = ~0;

  max_count = TYPE_MAXIMUM (off_t);

  /* The value -1 means to use DEFAULT_CONTEXT. */
  out_after = out_before = -1;
  /* Default before/after context: chaged by -C/-NUM options */
  default_context = 0;
  /* Changed by -o option */
  only_matching = 0;

  /* Internationalization. */
#if defined HAVE_SETLOCALE
  setlocale (LC_ALL, "");
#endif
#if defined ENABLE_NLS
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  exit_failure = EXIT_TROUBLE;
  atexit (clean_up_stdout);

  last_recursive = 0;
  prepended = prepend_default_options (getenv ("GREP_OPTIONS"), &argc, &argv);
  setmatcher (NULL);

  while (prev_optind = optind,
         (opt = get_nondigit_option (argc, argv, &default_context)) != -1)
    switch (opt)
      {
      case 'A':
        context_length_arg (optarg, &out_after);
        break;

      case 'B':
        context_length_arg (optarg, &out_before);
        break;

      case 'C':
        /* Set output match context, but let any explicit leading or
           trailing amount specified with -A or -B stand. */
        context_length_arg (optarg, &default_context);
        break;

      case 'D':
        if (STREQ (optarg, "read"))
          devices = READ_DEVICES;
        else if (STREQ (optarg, "skip"))
          devices = SKIP_DEVICES;
        else
          error (EXIT_TROUBLE, 0, _("unknown devices method"));
        break;

      case 'E':
        setmatcher ("egrep");
        break;

      case 'F':
        setmatcher ("fgrep");
        break;

      case 'P':
        setmatcher ("perl");
        break;

      case 'G':
        setmatcher ("grep");
        break;

      case 'X': /* undocumented on purpose */
        setmatcher (optarg);
        break;

      case 'H':
        with_filenames = 1;
        no_filenames = 0;
        break;

      case 'I':
        binary_files = WITHOUT_MATCH_BINARY_FILES;
        break;

      case 'T':
        align_tabs = 1;
        break;

      case 'U':
#if defined HAVE_DOS_FILE_CONTENTS
        dos_use_file_type = DOS_BINARY;
#endif
        break;

      case 'u':
#if defined HAVE_DOS_FILE_CONTENTS
        dos_report_unix_offset = 1;
#endif
        break;

      case 'V':
        show_version = 1;
        break;

      case 'a':
        binary_files = TEXT_BINARY_FILES;
        break;

      case 'b':
        out_byte = 1;
        break;

      case 'c':
        count_matches = 1;
        break;

      case 'd':
        directories = XARGMATCH ("--directories", optarg,
                                 directories_args, directories_types);
        if (directories == RECURSE_DIRECTORIES)
          last_recursive = prev_optind;
        break;

      case 'e':
        cc = strlen (optarg);
        keys = xrealloc (keys, keycc + cc + 1);
        strcpy (&keys[keycc], optarg);
        keycc += cc;
        keys[keycc++] = '\n';
        break;

      case 'f':
        fp = STREQ (optarg, "-") ? stdin : fopen (optarg, "r");
        if (!fp)
          error (EXIT_TROUBLE, errno, "%s", optarg);
        for (keyalloc = 1; keyalloc <= keycc + 1; keyalloc *= 2)
          ;
        keys = xrealloc (keys, keyalloc);
        oldcc = keycc;
        while (!feof (fp)
               && (cc = fread (keys + keycc, 1, keyalloc - 1 - keycc, fp)) > 0)
          {
            keycc += cc;
            if (keycc == keyalloc - 1)
              keys = x2nrealloc (keys, &keyalloc, sizeof *keys);
          }
        if (fp != stdin)
          fclose (fp);
        /* Append final newline if file ended in non-newline. */
        if (oldcc != keycc && keys[keycc - 1] != '\n')
          keys[keycc++] = '\n';
        break;

      case 'h':
        with_filenames = 0;
        no_filenames = 1;
        break;

      case 'i':
      case 'y':			/* For old-timers . . . */
        match_icase = 1;
        break;

      case 'L':
        /* Like -l, except list files that don't contain matches.
           Inspired by the same option in Hume's gre. */
        list_files = -1;
        break;

      case 'l':
        list_files = 1;
        break;

      case 'm':
        {
          uintmax_t value;
          switch (xstrtoumax (optarg, 0, 10, &value, ""))
            {
            case LONGINT_OK:
              max_count = value;
              if (0 <= max_count && max_count == value)
                break;
              /* Fall through.  */
            case LONGINT_OVERFLOW:
              max_count = TYPE_MAXIMUM (off_t);
              break;

            default:
              error (EXIT_TROUBLE, 0, _("invalid max count"));
            }
        }
        break;

      case 'n':
        out_line = 1;
        break;

      case 'o':
        only_matching = 1;
        break;

      case 'q':
        exit_on_match = 1;
        exit_failure = 0;
        break;

      case 'R':
      case 'r':
        directories = RECURSE_DIRECTORIES;
        last_recursive = prev_optind;
        break;

      case 's':
        suppress_errors = 1;
        break;

      case 'v':
        out_invert = 1;
        break;

      case 'w':
        match_words = 1;
        break;

      case 'x':
        match_lines = 1;
        break;

      case 'Z':
        filename_mask = 0;
        break;

      case 'z':
        eolbyte = '\0';
        break;

      case BINARY_FILES_OPTION:
        if (STREQ (optarg, "binary"))
          binary_files = BINARY_BINARY_FILES;
        else if (STREQ (optarg, "text"))
          binary_files = TEXT_BINARY_FILES;
        else if (STREQ (optarg, "without-match"))
          binary_files = WITHOUT_MATCH_BINARY_FILES;
        else
          error (EXIT_TROUBLE, 0, _("unknown binary-files type"));
        break;

      case COLOR_OPTION:
        if (optarg)
          {
            if (!strcasecmp (optarg, "always") || !strcasecmp (optarg, "yes")
                || !strcasecmp (optarg, "force"))
              color_option = 1;
            else if (!strcasecmp (optarg, "never") || !strcasecmp (optarg, "no")
                     || !strcasecmp (optarg, "none"))
              color_option = 0;
            else if (!strcasecmp (optarg, "auto") || !strcasecmp (optarg, "tty")
                     || !strcasecmp (optarg, "if-tty"))
              color_option = 2;
            else
              show_help = 1;
          }
        else
          color_option = 2;
        break;

      case EXCLUDE_OPTION:
        if (!excluded_patterns)
          excluded_patterns = new_exclude ();
        add_exclude (excluded_patterns, optarg, EXCLUDE_WILDCARDS);
        break;
      case EXCLUDE_FROM_OPTION:
        if (!excluded_patterns)
          excluded_patterns = new_exclude ();
        if (add_exclude_file (add_exclude, excluded_patterns, optarg,
                              EXCLUDE_WILDCARDS, '\n') != 0)
          {
            error (EXIT_TROUBLE, errno, "%s", optarg);
          }
        break;

      case EXCLUDE_DIRECTORY_OPTION:
        if (!excluded_directory_patterns)
          excluded_directory_patterns = new_exclude ();
        add_exclude (excluded_directory_patterns, optarg, EXCLUDE_WILDCARDS);
        break;

      case INCLUDE_OPTION:
        if (!included_patterns)
          included_patterns = new_exclude ();
        add_exclude (included_patterns, optarg,
                     EXCLUDE_WILDCARDS | EXCLUDE_INCLUDE);
        break;

      case GROUP_SEPARATOR_OPTION:
        group_separator = optarg;
        break;

      case LINE_BUFFERED_OPTION:
        line_buffered = 1;
        break;

      case LABEL_OPTION:
        label = optarg;
        break;

      case MMAP_OPTION:
        error (0, 0, _("the --mmap option has been a no-op since 2010"));
        break;

      case 0:
        /* long options */
        break;

      default:
        usage (EXIT_TROUBLE);
        break;

      }

  if (color_option == 2)
    color_option = isatty (STDOUT_FILENO) && should_colorize ();
  init_colorize ();

  /* POSIX.2 says that -q overrides -l, which in turn overrides the
     other output options.  */
  if (exit_on_match)
    list_files = 0;
  if (exit_on_match | list_files)
    {
      count_matches = 0;
      done_on_match = 1;
    }
  out_quiet = count_matches | done_on_match;

  if (out_after < 0)
    out_after = default_context;
  if (out_before < 0)
    out_before = default_context;

  if (color_option)
    {
      /* Legacy.  */
      char *userval = getenv ("GREP_COLOR");
      if (userval != NULL && *userval != '\0')
        selected_match_color = context_match_color = userval;

      /* New GREP_COLORS has priority.  */
      parse_grep_colors ();
    }

  if (show_version)
    {
      version_etc (stdout, program_name, PACKAGE_NAME, VERSION, AUTHORS,
                   (char *) NULL);
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    usage (EXIT_SUCCESS);

  struct stat tmp_stat;
  if (fstat (STDOUT_FILENO, &tmp_stat) == 0 && S_ISREG (tmp_stat.st_mode))
    out_stat = tmp_stat;

  if (keys)
    {
      if (keycc == 0)
        {
          /* No keys were specified (e.g. -f /dev/null).  Match nothing.  */
          out_invert ^= 1;
          match_lines = match_words = 0;
        }
      else
        /* Strip trailing newline. */
        --keycc;
    }
  else if (optind < argc)
    {
      /* A copy must be made in case of an xrealloc() or free() later.  */
      keycc = strlen (argv[optind]);
      keys = xmalloc (keycc + 1);
      strcpy (keys, argv[optind++]);
    }
  else
    usage (EXIT_TROUBLE);

  compile (keys, keycc);
  free (keys);

  if ((argc - optind > 1 && !no_filenames) || with_filenames)
    out_file = 1;

#ifdef SET_BINARY
  /* Output is set to binary mode because we shouldn't convert
     NL to CR-LF pairs, especially when grepping binary files.  */
  if (!isatty (1))
    SET_BINARY (1);
#endif

  if (max_count == 0)
    exit (EXIT_FAILURE);

  if (optind < argc)
    {
      status = 1;
      do
        {
          char *file = argv[optind];
          if (!STREQ (file, "-")
              && (included_patterns || excluded_patterns
                  || excluded_directory_patterns))
            {
              if (isdir (file))
                {
                  if (excluded_directory_patterns
                      && excluded_file_name (excluded_directory_patterns,
                                             file))
                    continue;
                }
              else
                {
                  if (included_patterns
                      && excluded_file_name (included_patterns, file))
                    continue;
                  if (excluded_patterns
                      && excluded_file_name (excluded_patterns, file))
                    continue;
                }
            }
          status &= grepfile (STREQ (file, "-") ? (char *) NULL : file,
                              &stats_base);
        }
      while (++optind < argc);
    }
  else if (directories == RECURSE_DIRECTORIES && prepended < last_recursive)
    {
      status = 1;
      if (stat (".", &stats_base.stat) == 0)
        status = grepdir (NULL, &stats_base);
      else
        suppressible_error (".", errno);
    }
  else
    status = grepfile ((char *) NULL, &stats_base);

  /* We register via atexit() to test stdout.  */
  exit (errseen ? EXIT_TROUBLE : status);
}