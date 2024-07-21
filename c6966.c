http_loop (struct url *u, struct url *original_url, char **newloc,
           char **local_file, const char *referer, int *dt, struct url *proxy,
           struct iri *iri)
{
  int count;
  bool got_head = false;         /* used for time-stamping and filename detection */
  bool time_came_from_head = false;
  bool got_name = false;
  char *tms;
  const char *tmrate;
  uerr_t err, ret = TRYLIMEXC;
  time_t tmr = -1;               /* remote time-stamp */
  struct http_stat hstat;        /* HTTP status */
  struct_stat st;
  bool send_head_first = true;
  bool force_full_retrieve = false;


  /* If we are writing to a WARC file: always retrieve the whole file. */
  if (opt.warc_filename != NULL)
    force_full_retrieve = true;


  /* Assert that no value for *LOCAL_FILE was passed. */
  assert (local_file == NULL || *local_file == NULL);

  /* Set LOCAL_FILE parameter. */
  if (local_file && opt.output_document)
    *local_file = HYPHENP (opt.output_document) ? NULL : xstrdup (opt.output_document);

  /* Reset NEWLOC parameter. */
  *newloc = NULL;

  /* This used to be done in main, but it's a better idea to do it
     here so that we don't go through the hoops if we're just using
     FTP or whatever. */
  if (opt.cookies)
    load_cookies ();

  /* Warn on (likely bogus) wildcard usage in HTTP. */
  if (opt.ftp_glob && has_wildcards_p (u->path))
    logputs (LOG_VERBOSE, _("Warning: wildcards not supported in HTTP.\n"));

  /* Setup hstat struct. */
  xzero (hstat);
  hstat.referer = referer;

  if (opt.output_document)
    {
      hstat.local_file = xstrdup (opt.output_document);
      got_name = true;
    }
  else if (!opt.content_disposition)
    {
      hstat.local_file =
        url_file_name (opt.trustservernames ? u : original_url, NULL);
      got_name = true;
    }

  if (got_name && file_exists_p (hstat.local_file) && opt.noclobber && !opt.output_document)
    {
      /* If opt.noclobber is turned on and file already exists, do not
         retrieve the file. But if the output_document was given, then this
         test was already done and the file didn't exist. Hence the !opt.output_document */
      get_file_flags (hstat.local_file, dt);
      ret = RETROK;
      goto exit;
    }

  /* Reset the counter. */
  count = 0;

  /* Reset the document type. */
  *dt = 0;

  /* Skip preliminary HEAD request if we're not in spider mode.  */
  if (!opt.spider)
    send_head_first = false;

  /* Send preliminary HEAD request if --content-disposition and -c are used
     together.  */
  if (opt.content_disposition && opt.always_rest)
    send_head_first = true;

#ifdef HAVE_METALINK
  if (opt.metalink_over_http)
    {
      *dt |= METALINK_METADATA;
      send_head_first = true;
    }
#endif

  if (opt.timestamping)
    {
      /* Use conditional get request if requested
       * and if timestamp is known at this moment.  */
      if (opt.if_modified_since && !send_head_first && got_name && file_exists_p (hstat.local_file))
        {
          *dt |= IF_MODIFIED_SINCE;
          {
            uerr_t timestamp_err = set_file_timestamp (&hstat);
            if (timestamp_err != RETROK)
              return timestamp_err;
          }
        }
        /* Send preliminary HEAD request if -N is given and we have existing
         * destination file or content disposition is enabled.  */
      else if (opt.content_disposition || file_exists_p (hstat.local_file))
        send_head_first = true;
    }

  /* THE loop */
  do
    {
      /* Increment the pass counter.  */
      ++count;
      sleep_between_retrievals (count);

      /* Get the current time string.  */
      tms = datetime_str (time (NULL));

      if (opt.spider && !got_head)
        logprintf (LOG_VERBOSE,
			  _("Spider mode enabled. Check if remote file exists.\n"));

      /* Print fetch message, if opt.verbose.  */
      if (opt.verbose)
        {
          char *hurl = url_string (u, URL_AUTH_HIDE_PASSWD);

          if (count > 1)
            {
              char tmp[256];
              sprintf (tmp, _("(try:%2d)"), count);
              logprintf (LOG_NOTQUIET, "--%s--  %s  %s\n",
                         tms, tmp, hurl);
            }
          else
            {
              logprintf (LOG_NOTQUIET, "--%s--  %s\n",
                         tms, hurl);
            }

#ifdef WINDOWS
          ws_changetitle (hurl);
#endif
          xfree (hurl);
        }

      /* Default document type is empty.  However, if spider mode is
         on or time-stamping is employed, HEAD_ONLY commands is
         encoded within *dt.  */
      if (send_head_first && !got_head)
        *dt |= HEAD_ONLY;
      else
        *dt &= ~HEAD_ONLY;

      /* Decide whether or not to restart.  */
      if (force_full_retrieve)
        hstat.restval = hstat.len;
      else if (opt.start_pos >= 0)
        hstat.restval = opt.start_pos;
      else if (opt.always_rest
          && got_name
          && stat (hstat.local_file, &st) == 0
          && S_ISREG (st.st_mode))
        /* When -c is used, continue from on-disk size.  (Can't use
           hstat.len even if count>1 because we don't want a failed
           first attempt to clobber existing data.)  */
        hstat.restval = st.st_size;
      else if (count > 1)
        /* otherwise, continue where the previous try left off */
        hstat.restval = hstat.len;
      else
        hstat.restval = 0;

      /* Decide whether to send the no-cache directive.  We send it in
         two cases:
           a) we're using a proxy, and we're past our first retrieval.
              Some proxies are notorious for caching incomplete data, so
              we require a fresh get.
           b) caching is explicitly inhibited. */
      if ((proxy && count > 1)        /* a */
          || !opt.allow_cache)        /* b */
        *dt |= SEND_NOCACHE;
      else
        *dt &= ~SEND_NOCACHE;

      /* Try fetching the document, or at least its head.  */
      err = gethttp (u, &hstat, dt, proxy, iri, count);

      /* Time?  */
      tms = datetime_str (time (NULL));

      /* Get the new location (with or without the redirection).  */
      if (hstat.newloc)
        *newloc = xstrdup (hstat.newloc);

      switch (err)
        {
        case HERR: case HEOF: case CONSOCKERR:
        case CONERROR: case READERR: case WRITEFAILED:
        case RANGEERR: case FOPEN_EXCL_ERR: case GATEWAYTIMEOUT:
          /* Non-fatal errors continue executing the loop, which will
             bring them to "while" statement at the end, to judge
             whether the number of tries was exceeded.  */
          printwhat (count, opt.ntry);
          continue;
        case FWRITEERR: case FOPENERR:
          /* Another fatal error.  */
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("Cannot write to %s (%s).\n"),
                     quote (hstat.local_file), strerror (errno));
        case HOSTERR: case CONIMPOSSIBLE: case PROXERR: case SSLINITFAILED:
        case CONTNOTSUPPORTED: case VERIFCERTERR: case FILEBADFILE:
        case UNKNOWNATTR:
          /* Fatal errors just return from the function.  */
          ret = err;
          goto exit;
        case ATTRMISSING:
          /* A missing attribute in a Header is a fatal Protocol error. */
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("Required attribute missing from Header received.\n"));
          ret = err;
          goto exit;
        case AUTHFAILED:
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("Username/Password Authentication Failed.\n"));
          ret = err;
          goto exit;
        case WARC_ERR:
          /* A fatal WARC error. */
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("Cannot write to WARC file.\n"));
          ret = err;
          goto exit;
        case WARC_TMP_FOPENERR: case WARC_TMP_FWRITEERR:
          /* A fatal WARC error. */
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("Cannot write to temporary WARC file.\n"));
          ret = err;
          goto exit;
        case CONSSLERR:
          /* Another fatal error.  */
          logprintf (LOG_NOTQUIET, _("Unable to establish SSL connection.\n"));
          ret = err;
          goto exit;
        case UNLINKERR:
          /* Another fatal error.  */
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("Cannot unlink %s (%s).\n"),
                     quote (hstat.local_file), strerror (errno));
          ret = err;
          goto exit;
        case NEWLOCATION:
        case NEWLOCATION_KEEP_POST:
          /* Return the new location to the caller.  */
          if (!*newloc)
            {
              logprintf (LOG_NOTQUIET,
                         _("ERROR: Redirection (%d) without location.\n"),
                         hstat.statcode);
              ret = WRONGCODE;
            }
          else
            {
              ret = err;
            }
          goto exit;
        case RETRUNNEEDED:
          /* The file was already fully retrieved. */
          ret = RETROK;
          goto exit;
        case RETRFINISHED:
          /* Deal with you later.  */
          break;
#ifdef HAVE_METALINK
        case RETR_WITH_METALINK:
          {
            if (hstat.metalink == NULL)
              {
                logputs (LOG_NOTQUIET,
                         _("Could not find Metalink data in HTTP response. "
                           "Downloading file using HTTP GET.\n"));
                *dt &= ~METALINK_METADATA;
                *dt &= ~HEAD_ONLY;
                got_head = true;
                continue;
              }

            logputs (LOG_VERBOSE,
                     _("Metalink headers found. "
                       "Switching to Metalink mode.\n"));

            ret = retrieve_from_metalink (hstat.metalink);
            goto exit;
          }
          break;
#endif
        default:
          /* All possibilities should have been exhausted.  */
          abort ();
        }

      if (!(*dt & RETROKF))
        {
          char *hurl = NULL;
          if (!opt.verbose)
            {
              /* #### Ugly ugly ugly! */
              hurl = url_string (u, URL_AUTH_HIDE_PASSWD);
              logprintf (LOG_NONVERBOSE, "%s:\n", hurl);
            }

          /* Fall back to GET if HEAD fails with a 500 or 501 error code. */
          if (*dt & HEAD_ONLY
              && (hstat.statcode == 500 || hstat.statcode == 501))
            {
              got_head = true;
              continue;
            }
          /* Maybe we should always keep track of broken links, not just in
           * spider mode.
           * Don't log error if it was UTF-8 encoded because we will try
           * once unencoded. */
          else if (opt.spider && !iri->utf8_encode)
            {
              /* #### Again: ugly ugly ugly! */
              if (!hurl)
                hurl = url_string (u, URL_AUTH_HIDE_PASSWD);
              nonexisting_url (hurl);
              logprintf (LOG_NOTQUIET, _("\
Remote file does not exist -- broken link!!!\n"));
            }
          else
            {
              logprintf (LOG_NOTQUIET, _("%s ERROR %d: %s.\n"),
                         tms, hstat.statcode,
                         quotearg_style (escape_quoting_style, hstat.error));
            }
          logputs (LOG_VERBOSE, "\n");
          ret = WRONGCODE;
          xfree (hurl);
          goto exit;
        }

      /* Did we get the time-stamp? */
      if (!got_head || (opt.spider && !opt.recursive))
        {
          got_head = true;    /* no more time-stamping */

          if (opt.timestamping && !hstat.remote_time)
            {
              logputs (LOG_NOTQUIET, _("\
Last-modified header missing -- time-stamps turned off.\n"));
            }
          else if (hstat.remote_time)
            {
              /* Convert the date-string into struct tm.  */
              tmr = http_atotm (hstat.remote_time);
              if (tmr == (time_t) (-1))
                logputs (LOG_VERBOSE, _("\
Last-modified header invalid -- time-stamp ignored.\n"));
              if (*dt & HEAD_ONLY)
                time_came_from_head = true;
            }

          if (send_head_first)
            {
              /* The time-stamping section.  */
              if (opt.timestamping)
                {
                  if (hstat.orig_file_name) /* Perform the following
                                               checks only if the file
                                               we're supposed to
                                               download already exists.  */
                    {
                      if (hstat.remote_time &&
                          tmr != (time_t) (-1))
                        {
                          /* Now time-stamping can be used validly.
                             Time-stamping means that if the sizes of
                             the local and remote file match, and local
                             file is newer than the remote file, it will
                             not be retrieved.  Otherwise, the normal
                             download procedure is resumed.  */
                          if (hstat.orig_file_tstamp >= tmr)
                            {
                              if (hstat.contlen == -1
                                  || hstat.orig_file_size == hstat.contlen)
                                {
                                  logprintf (LOG_VERBOSE, _("\
Server file no newer than local file %s -- not retrieving.\n\n"),
                                             quote (hstat.orig_file_name));
                                  ret = RETROK;
                                  goto exit;
                                }
                              else
                                {
                                  logprintf (LOG_VERBOSE, _("\
The sizes do not match (local %s) -- retrieving.\n"),
                                             number_to_static_string (hstat.orig_file_size));
                                }
                            }
                          else
                            {
                              force_full_retrieve = true;
                              logputs (LOG_VERBOSE,
                                       _("Remote file is newer, retrieving.\n"));
                            }

                          logputs (LOG_VERBOSE, "\n");
                        }
                    }

                  /* free_hstat (&hstat); */
                  hstat.timestamp_checked = true;
                }

              if (opt.spider)
                {
                  bool finished = true;
                  if (opt.recursive)
                    {
                      if (*dt & TEXTHTML)
                        {
                          logputs (LOG_VERBOSE, _("\
Remote file exists and could contain links to other resources -- retrieving.\n\n"));
                          finished = false;
                        }
                      else
                        {
                          logprintf (LOG_VERBOSE, _("\
Remote file exists but does not contain any link -- not retrieving.\n\n"));
                          ret = RETROK; /* RETRUNNEEDED is not for caller. */
                        }
                    }
                  else
                    {
                      if (*dt & TEXTHTML)
                        {
                          logprintf (LOG_VERBOSE, _("\
Remote file exists and could contain further links,\n\
but recursion is disabled -- not retrieving.\n\n"));
                        }
                      else
                        {
                          logprintf (LOG_VERBOSE, _("\
Remote file exists.\n\n"));
                        }
                      ret = RETROK; /* RETRUNNEEDED is not for caller. */
                    }

                  if (finished)
                    {
                      logprintf (LOG_NONVERBOSE,
                                 _("%s URL: %s %2d %s\n"),
                                 tms, u->url, hstat.statcode,
                                 hstat.message ? quotearg_style (escape_quoting_style, hstat.message) : "");
                      goto exit;
                    }
                }

              got_name = true;
              *dt &= ~HEAD_ONLY;
              count = 0;          /* the retrieve count for HEAD is reset */
              xfree (hstat.message);
              xfree (hstat.error);
              continue;
            } /* send_head_first */
        } /* !got_head */

      if (opt.useservertimestamps
          && (tmr != (time_t) (-1))
          && ((hstat.len == hstat.contlen) ||
              ((hstat.res == 0) && (hstat.contlen == -1))))
        {
          const char *fl = NULL;
          set_local_file (&fl, hstat.local_file);
          if (fl)
            {
              time_t newtmr = -1;
              /* Reparse time header, in case it's changed. */
              if (time_came_from_head
                  && hstat.remote_time && hstat.remote_time[0])
                {
                  newtmr = http_atotm (hstat.remote_time);
                  if (newtmr != (time_t)-1)
                    tmr = newtmr;
                }
              touch (fl, tmr);
            }
        }
      /* End of time-stamping section. */

      tmrate = retr_rate (hstat.rd_size, hstat.dltime);
      total_download_time += hstat.dltime;

      if (hstat.len == hstat.contlen)
        {
          if (*dt & RETROKF || opt.content_on_error)
            {
              bool write_to_stdout = (opt.output_document && HYPHENP (opt.output_document));

              logprintf (LOG_VERBOSE,
                         write_to_stdout
                         ? _("%s (%s) - written to stdout %s[%s/%s]\n\n")
                         : _("%s (%s) - %s saved [%s/%s]\n\n"),
                         tms, tmrate,
                         write_to_stdout ? "" : quote (hstat.local_file),
                         number_to_static_string (hstat.len),
                         number_to_static_string (hstat.contlen));
              logprintf (LOG_NONVERBOSE,
                         "%s URL:%s [%s/%s] -> \"%s\" [%d]\n",
                         tms, u->url,
                         number_to_static_string (hstat.len),
                         number_to_static_string (hstat.contlen),
                         hstat.local_file, count);
            }
          ++numurls;
          total_downloaded_bytes += hstat.rd_size;

          /* Remember that we downloaded the file for later ".orig" code. */
          if (*dt & ADDED_HTML_EXTENSION)
            downloaded_file (FILE_DOWNLOADED_AND_HTML_EXTENSION_ADDED, hstat.local_file);
          else
            downloaded_file (FILE_DOWNLOADED_NORMALLY, hstat.local_file);

          ret = RETROK;
          goto exit;
        }
      else if (hstat.res == 0) /* No read error */
        {
          if (hstat.contlen == -1)  /* We don't know how much we were supposed
                                       to get, so assume we succeeded. */
            {
              if (*dt & RETROKF || opt.content_on_error)
                {
                  bool write_to_stdout = (opt.output_document && HYPHENP (opt.output_document));

                  logprintf (LOG_VERBOSE,
                             write_to_stdout
                             ? _("%s (%s) - written to stdout %s[%s]\n\n")
                             : _("%s (%s) - %s saved [%s]\n\n"),
                             tms, tmrate,
                             write_to_stdout ? "" : quote (hstat.local_file),
                             number_to_static_string (hstat.len));
                  logprintf (LOG_NONVERBOSE,
                             "%s URL:%s [%s] -> \"%s\" [%d]\n",
                             tms, u->url, number_to_static_string (hstat.len),
                             hstat.local_file, count);
                }
              ++numurls;
              total_downloaded_bytes += hstat.rd_size;

              /* Remember that we downloaded the file for later ".orig" code. */
              if (*dt & ADDED_HTML_EXTENSION)
                downloaded_file (FILE_DOWNLOADED_AND_HTML_EXTENSION_ADDED, hstat.local_file);
              else
                downloaded_file (FILE_DOWNLOADED_NORMALLY, hstat.local_file);

              ret = RETROK;
              goto exit;
            }
          else if (hstat.len < hstat.contlen) /* meaning we lost the
                                                 connection too soon */
            {
              logprintf (LOG_VERBOSE,
                         _("%s (%s) - Connection closed at byte %s. "),
                         tms, tmrate, number_to_static_string (hstat.len));
              printwhat (count, opt.ntry);
              continue;
            }
          else if (hstat.len != hstat.restval)
            /* Getting here would mean reading more data than
               requested with content-length, which we never do.  */
            abort ();
          else
            {
              /* Getting here probably means that the content-length was
               * _less_ than the original, local size. We should probably
               * truncate or re-read, or something. FIXME */
              ret = RETROK;
              goto exit;
            }
        }
      else /* from now on hstat.res can only be -1 */
        {
          if (hstat.contlen == -1)
            {
              logprintf (LOG_VERBOSE,
                         _("%s (%s) - Read error at byte %s (%s)."),
                         tms, tmrate, number_to_static_string (hstat.len),
                         hstat.rderrmsg);
              printwhat (count, opt.ntry);
              continue;
            }
          else /* hstat.res == -1 and contlen is given */
            {
              logprintf (LOG_VERBOSE,
                         _("%s (%s) - Read error at byte %s/%s (%s). "),
                         tms, tmrate,
                         number_to_static_string (hstat.len),
                         number_to_static_string (hstat.contlen),
                         hstat.rderrmsg);
              printwhat (count, opt.ntry);
              continue;
            }
        }
      /* not reached */
    }
  while (!opt.ntry || (count < opt.ntry));

exit:
  if ((ret == RETROK || opt.content_on_error) && local_file)
    {
      xfree (*local_file);
      *local_file = xstrdup (hstat.local_file);
    }
  free_hstat (&hstat);

  return ret;
}