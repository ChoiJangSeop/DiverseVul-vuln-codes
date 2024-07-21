main (
  int argc,
  char *argv[]
)
{
  int exit_val;
  fribidi_boolean file_found;
  char *s;
  FILE *IN;

  text_width = default_text_width;
  do_break = true;
  do_pad = true;
  do_mirror = true;
  do_clean = false;
  do_reorder_nsm = false;
  show_input = false;
  show_visual = true;
  show_basedir = false;
  show_ltov = false;
  show_vtol = false;
  show_levels = false;
  char_set = "UTF-8";
  bol_text = NULL;
  eol_text = NULL;
  input_base_direction = FRIBIDI_PAR_ON;

  if ((s = (char *) getenv ("COLUMNS")))
    {
      int i;

      i = atoi (s);
      if (i > 0)
	text_width = i;
    }

#define CHARSETDESC 257
#define CAPRTL 258

  /* Parse the command line with getopt library */
  /* Must set argv[0], getopt uses it to generate error messages */
  argv[0] = appname;
  while (1)
    {
      int option_index = 0, c;
      static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"version", 0, 0, 'V'},
	{"verbose", 0, 0, 'v'},
	{"debug", 0, 0, 'd'},
	{"test", 0, 0, 't'},
	{"charset", 1, 0, 'c'},
	{"charsetdesc", 1, 0, CHARSETDESC},
	{"caprtl", 0, 0, CAPRTL},
	{"showinput", 0, (int *) (void *) &show_input, true},
	{"nopad", 0, (int *) (void *) &do_pad, false},
	{"nobreak", 0, (int *) (void *) &do_break, false},
	{"width", 1, 0, 'w'},
	{"bol", 1, 0, 'B'},
	{"eol", 1, 0, 'E'},
	{"nomirror", 0, (int *) (void *) &do_mirror, false},
	{"reordernsm", 0, (int *) (void *) &do_reorder_nsm, true},
	{"clean", 0, (int *) (void *) &do_clean, true},
	{"ltr", 0, (int *) (void *) &input_base_direction, FRIBIDI_PAR_LTR},
	{"rtl", 0, (int *) (void *) &input_base_direction, FRIBIDI_PAR_RTL},
	{"wltr", 0, (int *) (void *) &input_base_direction,
	 FRIBIDI_PAR_WLTR},
	{"wrtl", 0, (int *) (void *) &input_base_direction,
	 FRIBIDI_PAR_WRTL},
	{"basedir", 0, (int *) (void *) &show_basedir, true},
	{"ltov", 0, (int *) (void *) &show_ltov, true},
	{"vtol", 0, (int *) (void *) &show_vtol, true},
	{"levels", 0, (int *) (void *) &show_levels, true},
	{"novisual", 0, (int *) (void *) &show_visual, false},
	{0, 0, 0, 0}
      };

      c =
	getopt_long (argc, argv, "hVvdtc:w:B:E:", long_options,
		     &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  break;
	case 'h':
	  help ();
	  break;
	case 'V':
	  version ();
	  break;
	case 'v':
	  show_basedir = show_ltov = show_vtol = show_levels = true;
	  break;
	case 'w':
	  text_width = atoi (optarg);
	  if (text_width <= 0)
	    die2 ("invalid screen width `%s'\n", optarg);
	  break;
	case 'B':
	  bol_text = optarg;
	  break;
	case 'E':
	  eol_text = optarg;
	  break;
	case 'd':
	  if (!fribidi_set_debug (true))
	    die1
	      ("lib" FRIBIDI
	       " must be compiled with DEBUG option to enable\nturn debug info on.\n");
	  break;
	case 't':
	  do_clean = show_input = do_reorder_nsm = true;
	  do_break = false;
	  text_width = default_text_width;
	  break;
	case 'c':
	  char_set = my_fribidi_strdup (optarg);
	  if (!char_set)
	    die1 ("memory allocation failed for char_set!");
	  break;
	case CAPRTL:
	  char_set = "CapRTL";
	  break;
	case CHARSETDESC:
	  char_set = optarg;
	  char_set_num = fribidi_parse_charset (char_set);
	  if (!char_set_num)
	    die2 ("unrecognized character set `%s'\n", char_set);
	  if (!fribidi_char_set_desc (char_set_num))
	    die2 ("no description available for character set `%s'\n",
		  fribidi_char_set_name (char_set_num));
	  else
	    printf ("Descriptions for character set %s:\n"
		    "\n" "%s", fribidi_char_set_title (char_set_num),
		    fribidi_char_set_desc (char_set_num));
	  exit (0);
	  break;
	case ':':
	case '?':
	  die2 (NULL, NULL);
	  break;
	default:
	  break;
	}
    }

  char_set_num = fribidi_parse_charset (char_set);

  if (!char_set_num)
    die2 ("unrecognized character set `%s'\n", char_set);

FRIBIDI_BEGIN_IGNORE_DEPRECATIONS
  fribidi_set_mirroring (do_mirror);
  fribidi_set_reorder_nsm (do_reorder_nsm);
FRIBIDI_END_IGNORE_DEPRECATIONS
  exit_val = 0;
  file_found = false;
  while (optind < argc || !file_found)
    {
      const char *filename;

      filename = optind < argc ? argv[optind++] : "-";
      file_found = true;

      /* Open the infile for reading */
      if (filename[0] == '-' && !filename[1])
	{
	  IN = stdin;
	}
      else
	{
	  IN = fopen (filename, "r");
	  if (!IN)
	    {
	      fprintf (stderr, "%s: %s: no such file or directory\n",
		       appname, filename);
	      exit_val = 1;
	      continue;
	    }
	}

      /* Read and process input one line at a time */
      {
	char S_[MAX_STR_LEN];
	int padding_width, break_width;

	padding_width = show_input ? (text_width - 10) / 2 : text_width;
	break_width = do_break ? padding_width : 3 * MAX_STR_LEN;

	while (fgets (S_, sizeof (S_) - 1, IN))
	  {
	    const char *new_line, *nl_found;
	    FriBidiChar logical[MAX_STR_LEN];
	    char outstring[MAX_STR_LEN];
	    FriBidiParType base;
	    FriBidiStrIndex len;

	    nl_found = "";
	    S_[sizeof (S_) - 1] = 0;
	    len = strlen (S_);
	    /* chop */
	    if (S_[len - 1] == '\n')
	      {
		len--;
		S_[len] = '\0';
		new_line = "\n";
	      }
	    else
	      new_line = "";
	    /* TODO: handle \r */

	    len = fribidi_charset_to_unicode (char_set_num, S_, len, logical);

	    {
	      FriBidiChar *visual;
	      FriBidiStrIndex *ltov, *vtol;
	      FriBidiLevel *levels;
	      fribidi_boolean log2vis;

	      visual = show_visual ? ALLOCATE (FriBidiChar,
					       len + 1
	      ) : NULL;
	      ltov = show_ltov ? ALLOCATE (FriBidiStrIndex,
					   len + 1
	      ) : NULL;
	      vtol = show_vtol ? ALLOCATE (FriBidiStrIndex,
					   len + 1
	      ) : NULL;
	      levels = show_levels ? ALLOCATE (FriBidiLevel,
					       len + 1
	      ) : NULL;

	      /* Create a bidi string. */
	      base = input_base_direction;

	      log2vis = fribidi_log2vis (logical, len, &base,
					 /* output */
					 visual, ltov, vtol, levels);

	      if (log2vis)
		{

		  if (show_input)
		    printf ("%-*s => ", padding_width, S_);

		  /* Remove explicit marks, if asked for. */

		  if (do_clean)
		    len =
		      fribidi_remove_bidi_marks (visual, len, ltov, vtol,
						 levels);

		  if (show_visual)
		    {
		      printf ("%s", nl_found);

		      if (bol_text)
			printf ("%s", bol_text);

		      /* Convert it to input charset and print. */
		      {
			FriBidiStrIndex idx, st;
			for (idx = 0; idx < len;)
			  {
			    FriBidiStrIndex wid, inlen;

			    wid = break_width;
			    st = idx;
			    if (char_set_num != FRIBIDI_CHAR_SET_CAP_RTL)
			      while (wid > 0 && idx < len)
				{
				  wid -=
				    FRIBIDI_IS_EXPLICIT_OR_ISOLATE_OR_BN_OR_NSM
				    (fribidi_get_bidi_type (visual[idx])) ? 0
				    : 1;
				  idx++;
				}
			    else
			      while (wid > 0 && idx < len)
				{
				  wid--;
				  idx++;
				}
			    if (wid < 0 && idx - st > 1)
			      idx--;
			    inlen = idx - st;

			    fribidi_unicode_to_charset (char_set_num,
							visual + st, inlen,
							outstring);
			    if (FRIBIDI_IS_RTL (base))
			      printf ("%*s",
				      (int) (do_pad ? (padding_width +
						       strlen (outstring) -
						       (break_width -
							wid)) : 0),
				      outstring);
			    else
			      printf ("%s", outstring);
			    if (idx < len)
			      printf ("\n");
			  }
		      }
		      if (eol_text)
			printf ("%s", eol_text);

		      nl_found = "\n";
		    }
		  if (show_basedir)
		    {
		      printf ("%s", nl_found);
		      printf ("Base direction: %s",
			      (FRIBIDI_DIR_TO_LEVEL (base) ? "R" : "L"));
		      nl_found = "\n";
		    }
		  if (show_ltov)
		    {
		      FriBidiStrIndex i;

		      printf ("%s", nl_found);
		      for (i = 0; i < len; i++)
			printf ("%ld ", (long) ltov[i]);
		      nl_found = "\n";
		    }
		  if (show_vtol)
		    {
		      FriBidiStrIndex i;

		      printf ("%s", nl_found);
		      for (i = 0; i < len; i++)
			printf ("%ld ", (long) vtol[i]);
		      nl_found = "\n";
		    }
		  if (show_levels)
		    {
		      FriBidiStrIndex i;

		      printf ("%s", nl_found);
		      for (i = 0; i < len; i++)
			printf ("%d ", (int) levels[i]);
		      nl_found = "\n";
		    }
		}
	      else
		{
		  exit_val = 2;
		}

	      if (show_visual)
		free (visual);
	      if (show_ltov)
		free (ltov);
	      if (show_vtol)
		free (vtol);
	      if (show_levels)
		free (levels);
	    }

	    if (*nl_found)
	      printf ("%s", new_line);
	  }
      }
    }

  return exit_val;
}