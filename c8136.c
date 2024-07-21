usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [STRINGS]...\n\
"), program_name);
      fputs (_("\
Internationalized Domain Name (IDNA2008) convert STRINGS, or standard input.\n\
\n\
"), stdout);
      fputs (_("\
Command line interface to the Libidn2 implementation of IDNA2008.\n\
\n\
All strings are expected to be encoded in the locale charset.\n\
\n\
To process a string that starts with `-', for example `-foo', use `--'\n\
to signal the end of parameters, as in `idn2 --quiet -- -foo'.\n\
\n\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -h, --help               Print help and exit\n\
  -V, --version            Print version and exit\n\
"), stdout);
      fputs (_("\
  -d, --decode             Decode (punycode) domain name\n\
  -l, --lookup             Lookup domain name (default)\n\
  -r, --register           Register label\n\
"), stdout);
      fputs (_("\
  -T, --tr46t              Enable TR46 transitional processing\n\
  -N, --tr46nt             Enable TR46 non-transitional processing\n\
      --no-tr46            Disable TR46 processing\n\
"), stdout);
      fputs (_("\
      --usestd3asciirules  Enable STD3 ASCII rules\n\
      --debug              Print debugging information\n\
      --quiet              Silent operation\n\
"), stdout);
      emit_bug_reporting_address ();
    }
  exit (status);
}