main(int argc, char *argv[])
{
  int i, c;
  FILE *ifp = 0, *ofp = 0;
  const char *ifp_filename = "<stdin>";
  const char *ofp_filename = "<stdout>";
  const char *set_font_name = 0;
  struct font_reader fr;
  uint32_t rfork_len;
  int raw = 0, macbinary = 1, applesingle = 0, appledouble = 0, binhex = 0;

  Clp_Parser *clp =
    Clp_NewParser(argc, (const char * const *)argv, sizeof(options) / sizeof(options[0]), options);
  program_name = Clp_ProgramName(clp);

  /* interpret command line arguments using CLP */
  while (1) {
    int opt = Clp_Next(clp);
    switch (opt) {

     case RAW_OPT:
      raw = 1;
      macbinary = applesingle = appledouble = binhex = 0;
      break;

     case MACBINARY_OPT:
      macbinary = 1;
      raw = applesingle = appledouble = binhex = 0;
      break;

     case APPLESINGLE_OPT:
      applesingle = 1;
      raw = macbinary = appledouble = binhex = 0;
      break;

     case APPLEDOUBLE_OPT:
      appledouble = 1;
      raw = macbinary = applesingle = binhex = 0;
      break;

     case BINHEX_OPT:
      binhex = 1;
      raw = macbinary = applesingle = appledouble = 0;
      break;

     output_file:
     case OUTPUT_OPT:
      if (ofp)
	fatal_error("output file already specified");
      if (strcmp(clp->vstr, "-") == 0)
	ofp = stdout;
      else {
	ofp_filename = clp->vstr;
	ofp = fopen(ofp_filename, "wb");
	if (!ofp) fatal_error("%s: %s", ofp_filename, strerror(errno));
      }
      break;

     case FILENAME_OPT:
      if (set_font_name)
	fatal_error("Macintosh font filename already specified");
      set_font_name = clp->vstr;
      break;

     case HELP_OPT:
      usage();
      exit(0);
      break;

     case VERSION_OPT:
      printf("t1mac (LCDF t1utils) %s\n", VERSION);
      printf("Copyright (C) 2000-2010 Eddie Kohler et al.\n\
This is free software; see the source for copying conditions.\n\
There is NO warranty, not even for merchantability or fitness for a\n\
particular purpose.\n");
      exit(0);
      break;

     case Clp_NotOption:
      if (ifp && ofp)
	fatal_error("too many arguments");
      else if (ifp)
	goto output_file;
      if (strcmp(clp->vstr, "-") == 0)
	ifp = stdin;
      else {
	ifp_filename = clp->vstr;
	ifp = fopen(clp->vstr, "r");
	if (!ifp) fatal_error("%s: %s", clp->vstr, strerror(errno));
      }
      break;

     case Clp_Done:
      goto done;

     case Clp_BadOption:
      short_usage();
      exit(1);
      break;

    }
  }

 done:
  if (!ifp) ifp = stdin;
  if (!ofp) ofp = stdout;

#if defined(_MSDOS) || defined(_WIN32)
  /* As we are processing a PFB (binary) output */
  /* file, we must set its file mode to binary. */
  _setmode(_fileno(ofp), _O_BINARY);
#endif

  /* prepare font reader */
  fr.output_ascii = t1mac_output_ascii;
  fr.output_binary = t1mac_output_binary;
  fr.output_end = t1mac_output_end;

  /* prepare resource fork file */
  rfork_f = tmpfile();
  if (!rfork_f)
    fatal_error("cannot open temorary file: %s", strerror(errno));
  for (i = 0; i < RFORK_HEADERLEN; i++)
    putc(0, rfork_f);
  init_current_post();

  /* peek at first byte to see if it is the PFB marker 0x80 */
  c = getc(ifp);
  ungetc(c, ifp);

  /* do the file */
  if (c == PFB_MARKER)
    process_pfb(ifp, ifp_filename, &fr);
  else if (c == '%')
    process_pfa(ifp, ifp_filename, &fr);
  else
    fatal_error("%s does not start with font marker (`%%' or 0x80)", ifp_filename);
  if (ifp != stdin)
    fclose(ifp);

  /* check if anything was read */
  if (nrsrc == 0)
    error("no POST resources written -- are you sure this was a font?");

  /* output large B/W icon */
  output_new_rsrc("ICN#", 256, 32, (const char *)icon_bw_data, 256);
  /* output FREF */
  output_new_rsrc("FREF", 256, 32, "LWFN\0\0\0", 7);
  /* output BNDL */
  output_new_rsrc("BNDL", 256, 32, "T1UT\0\0\0\1FREF\0\0\0\0\1\0ICN#\0\0\0\0\1\0", 28);
  /* output other icons */
  output_new_rsrc("icl8", 256, 32, (const char *)icon_8_data, 1024);
  output_new_rsrc("icl4", 256, 32, (const char *)icon_4_data, 512);
  output_new_rsrc("ics#", 256, 32, (const char *)small_icon_bw_data, 64);
  output_new_rsrc("ics8", 256, 32, (const char *)small_icon_8_data, 256);
  output_new_rsrc("ics4", 256, 32, (const char *)small_icon_4_data, 128);
  /* output T1UT (signature) */
  output_new_rsrc("T1UT", 0, 0, "DConverted by t1mac (t1utils) \251Eddie Kohler http://www.lcdf.org/type/", 69);

  /* finish off resource file */
  rfork_len = complete_rfork();

  /* prepare font name */
  if (!set_font_name && font_name) {
    int part = 0, len = 0;
    char *x, *s;
    for (x = s = font_name; *s; s++)
      if (isupper(*s) || isdigit(*s)) {
	*x++ = *s;
	part++;
	len = 1;
      } else if (islower(*s)) {
	if (len < (part <= 1 ? 5 : 3))
	  *x++ = *s;
	len++;
      }
    *x++ = 0;
    set_font_name = font_name;
  } else if (!set_font_name)
    set_font_name = "Unknown Font";

  /* now, output the file */
  if (macbinary)
    output_macbinary(rfork_f, rfork_len, set_font_name, ofp);
  else if (raw)
    output_raw(rfork_f, rfork_len, ofp);
  else if (applesingle || appledouble)
    output_applesingle(rfork_f, rfork_len, set_font_name, ofp, appledouble);
  else if (binhex)
    output_binhex(rfork_f, rfork_len, set_font_name, ofp);
  else
    fatal_error("strange output format");
  fclose(rfork_f);

  if (ofp != stdout)
    fclose(ofp);
  return 0;
}