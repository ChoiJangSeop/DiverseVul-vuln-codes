ftp_loop (struct url *u, char **local_file, int *dt, struct url *proxy, 
          bool recursive, bool glob)
{
  ccon con;                     /* FTP connection */
  uerr_t res;

  *dt = 0;

  xzero (con);

  con.csock = -1;
  con.st = ON_YOUR_OWN;
  con.rs = ST_UNIX;
  con.id = NULL;
  con.proxy = proxy;

  /* If the file name is empty, the user probably wants a directory
     index.  We'll provide one, properly HTML-ized.  Unless
     opt.htmlify is 0, of course.  :-) */
  if (!*u->file && !recursive)
    {
      struct fileinfo *f;
      res = ftp_get_listing (u, &con, &f);

      if (res == RETROK)
        {
          if (opt.htmlify && !opt.spider)
            {
              char *filename = (opt.output_document
                                ? xstrdup (opt.output_document)
                                : (con.target ? xstrdup (con.target)
                                   : url_file_name (u)));
              res = ftp_index (filename, u, f);
              if (res == FTPOK && opt.verbose)
                {
                  if (!opt.output_document)
                    {
                      struct_stat st;
                      wgint sz;
                      if (stat (filename, &st) == 0)
                        sz = st.st_size;
                      else
                        sz = -1;
                      logprintf (LOG_NOTQUIET,
                                 _("Wrote HTML-ized index to %s [%s].\n"),
                                 quote (filename), number_to_static_string (sz));
                    }
                  else
                    logprintf (LOG_NOTQUIET,
                               _("Wrote HTML-ized index to %s.\n"),
                               quote (filename));
                }
              xfree (filename);
            }
          freefileinfo (f);
        }
    }
  else
    {
      bool ispattern = false;
      if (glob)
        {
          /* Treat the URL as a pattern if the file name part of the
             URL path contains wildcards.  (Don't check for u->file
             because it is unescaped and therefore doesn't leave users
             the option to escape literal '*' as %2A.)  */
          char *file_part = strrchr (u->path, '/');
          if (!file_part)
            file_part = u->path;
          ispattern = has_wildcards_p (file_part);
        }
      if (ispattern || recursive || opt.timestamping)
        {
          /* ftp_retrieve_glob is a catch-all function that gets called
             if we need globbing, time-stamping or recursion.  Its
             third argument is just what we really need.  */
          res = ftp_retrieve_glob (u, &con,
                                   ispattern ? GLOB_GLOBALL : GLOB_GETONE);
        }
      else
        res = ftp_loop_internal (u, NULL, &con, local_file);
    }
  if (res == FTPOK)
    res = RETROK;
  if (res == RETROK)
    *dt |= RETROKF;
  /* If a connection was left, quench it.  */
  if (con.csock != -1)
    fd_close (con.csock);
  xfree_null (con.id);
  con.id = NULL;
  xfree_null (con.target);
  con.target = NULL;
  return res;
}