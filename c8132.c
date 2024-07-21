main (int argc, char *argv[])
{
  unsigned cmdn;
  int flags = IDN2_NONTRANSITIONAL;

  setlocale (LC_ALL, "");
  set_program_name (argv[0]);
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  if (cmdline_parser (argc, argv, &args_info) != 0)
    return EXIT_FAILURE;

  if (args_info.version_given)
    {
      version_etc (stdout, "idn2", PACKAGE_NAME, VERSION,
		   "Simon Josefsson", (char *) NULL);
      return EXIT_SUCCESS;
    }

  if (args_info.help_given)
    usage (EXIT_SUCCESS);

  if (!args_info.quiet_given
      && args_info.inputs_num == 0 && isatty (fileno (stdin)))
    fprintf (stderr, "%s %s\n" GREETING, PACKAGE, VERSION);

  if (args_info.debug_given)
    fprintf (stderr, _("Charset: %s\n"), locale_charset ());

  if (!args_info.quiet_given
      && args_info.inputs_num == 0 && isatty (fileno (stdin)))
    fprintf (stderr, "%s", _("Type each input string on a line by itself, "
			     "terminated by a newline character.\n"));

  if (args_info.tr46t_given)
    flags = IDN2_TRANSITIONAL;
  else if (args_info.tr46nt_given)
    flags = IDN2_NONTRANSITIONAL;
  else if (args_info.no_tr46_given)
    flags = IDN2_NO_TR46;

  if (flags && args_info.usestd3asciirules_given)
    flags |= IDN2_USE_STD3_ASCII_RULES;

  for (cmdn = 0; cmdn < args_info.inputs_num; cmdn++)
    process_input (args_info.inputs[cmdn], flags | IDN2_NFC_INPUT);

  if (!cmdn)
    {
      char *buf = NULL;
      size_t bufsize = 0;

      while (getline (&buf, &bufsize, stdin) > 0)
	process_input (buf, flags);

      free (buf);
    }

  if (ferror (stdin))
    error (EXIT_FAILURE, errno, "%s", _("input error"));

  cmdline_parser_free (&args_info);

  return EXIT_SUCCESS;
}