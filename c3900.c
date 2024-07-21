url_file_name (const struct url *u, char *replaced_filename)
{
  struct growable fnres;        /* stands for "file name result" */
  struct growable temp_fnres;

  const char *u_file;
  char *fname, *unique, *fname_len_check;
  const char *index_filename = "index.html"; /* The default index file is index.html */
  size_t max_length;

  fnres.base = NULL;
  fnres.size = 0;
  fnres.tail = 0;

  temp_fnres.base = NULL;
  temp_fnres.size = 0;
  temp_fnres.tail = 0;

  /* If an alternative index file was defined, change index_filename */
  if (opt.default_page)
    index_filename = opt.default_page;


  /* Start with the directory prefix, if specified. */
  if (opt.dir_prefix)
    append_string (opt.dir_prefix, &fnres);

  /* If "dirstruct" is turned on (typically the case with -r), add
     the host and port (unless those have been turned off) and
     directory structure.  */
  if (opt.dirstruct)
    {
      if (opt.protocol_directories)
        {
          if (fnres.tail)
            append_char ('/', &fnres);
          append_string (supported_schemes[u->scheme].name, &fnres);
        }
      if (opt.add_hostdir)
        {
          if (fnres.tail)
            append_char ('/', &fnres);
          if (0 != strcmp (u->host, ".."))
            append_string (u->host, &fnres);
          else
            /* Host name can come from the network; malicious DNS may
               allow ".." to be resolved, causing us to write to
               "../<file>".  Defang such host names.  */
            append_string ("%2E%2E", &fnres);
          if (u->port != scheme_default_port (u->scheme))
            {
              char portstr[24];
              number_to_string (portstr, u->port);
              append_char (FN_PORT_SEP, &fnres);
              append_string (portstr, &fnres);
            }
        }

      append_dir_structure (u, &fnres);
    }

  if (!replaced_filename)
    {
      /* Create the filename. */
      u_file = *u->file ? u->file : index_filename;

      /* Append "?query" to the file name, even if empty,
       * and create fname_len_check. */
      if (u->query)
        fname_len_check = concat_strings (u_file, FN_QUERY_SEP_STR, u->query, NULL);
      else
        fname_len_check = strdupdelim (u_file, u_file + strlen (u_file));
    }
  else
    {
      u_file = replaced_filename;
      fname_len_check = strdupdelim (u_file, u_file + strlen (u_file));
    }

  append_uri_pathel (fname_len_check,
    fname_len_check + strlen (fname_len_check), false, &temp_fnres);

  /* Zero-terminate the temporary file name. */
  append_char ('\0', &temp_fnres);

  /* Check that the length of the file name is acceptable. */
#ifdef WINDOWS
  if (MAX_PATH > (fnres.tail + CHOMP_BUFFER + 2))
    {
      max_length = MAX_PATH - (fnres.tail + CHOMP_BUFFER + 2);
      /* FIXME: In Windows a filename is usually limited to 255 characters.
      To really be accurate you could call GetVolumeInformation() to get
      lpMaximumComponentLength
      */
      if (max_length > 255)
        {
          max_length = 255;
        }
    }
  else
    {
      max_length = 0;
    }
#else
  max_length = get_max_length (fnres.base, fnres.tail, _PC_NAME_MAX) - CHOMP_BUFFER;
#endif
  if (max_length > 0 && strlen (temp_fnres.base) > max_length)
    {
      logprintf (LOG_NOTQUIET, "The name is too long, %lu chars total.\n",
          (unsigned long) strlen (temp_fnres.base));
      logprintf (LOG_NOTQUIET, "Trying to shorten...\n");

      /* Shorten the file name. */
      temp_fnres.base[max_length] = '\0';

      logprintf (LOG_NOTQUIET, "New name is %s.\n", temp_fnres.base);
    }

  xfree (fname_len_check);

  /* The filename has already been 'cleaned' by append_uri_pathel() above.  So,
   * just append it. */
  if (fnres.tail)
    append_char ('/', &fnres);
  append_string (temp_fnres.base, &fnres);

  fname = fnres.base;

  /* Make a final check that the path length is acceptable? */
  /* TODO: check fnres.base for path length problem */

  xfree (temp_fnres.base);

  /* Check the cases in which the unique extensions are not used:
     1) Clobbering is turned off (-nc).
     2) Retrieval with regetting.
     3) Timestamping is used.
     4) Hierarchy is built.
     5) Backups are specified.

     The exception is the case when file does exist and is a
     directory (see `mkalldirs' for explanation).  */

  if (ALLOW_CLOBBER
      && !(file_exists_p (fname) && !file_non_directory_p (fname)))
    {
      unique = fname;
    }
  else
    {
      unique = unique_name (fname, true);
      if (unique != fname)
        xfree (fname);
    }

/* On VMS, alter the name as required. */
#ifdef __VMS
  {
    char *unique2;

    unique2 = ods_conform( unique);
    if (unique2 != unique)
      {
        xfree (unique);
        unique = unique2;
      }
  }
#endif /* def __VMS */

  return unique;
}