getftp (struct url *u, struct url *original_url,
        wgint passed_expected_bytes, wgint *qtyread,
        wgint restval, ccon *con, int count, wgint *last_expected_bytes,
        FILE *warc_tmp)
{
  int csock, dtsock, local_sock, res;
  uerr_t err = RETROK;          /* appease the compiler */
  FILE *fp = NULL;
  char *respline, *tms;
  const char *user, *passwd, *tmrate;
  int cmd = con->cmd;
  wgint expected_bytes = 0;
  bool got_expected_bytes = false;
  bool rest_failed = false;
  int flags;
  wgint rd_size, previous_rd_size = 0;
  char type_char;
  bool try_again;
  bool list_a_used = false;
#ifdef HAVE_SSL
  enum prot_level prot = (opt.ftps_clear_data_connection ? PROT_CLEAR : PROT_PRIVATE);
  /* these variables tell whether the target server
   * accepts the security extensions (RFC 2228) or not,
   * and whether we're actually using any of them
   * (encryption at the control connection only,
   * or both at control and data connections) */
  bool using_control_security = false, using_data_security = false;
#endif

  assert (con != NULL);
  assert (con->target != NULL);

  /* Debug-check of the sanity of the request by making sure that LIST
     and RETR are never both requested (since we can handle only one
     at a time.  */
  assert (!((cmd & DO_LIST) && (cmd & DO_RETR)));
  /* Make sure that at least *something* is requested.  */
  assert ((cmd & (DO_LIST | DO_CWD | DO_RETR | DO_LOGIN)) != 0);

  *qtyread = restval;

  /* Find the username with priority */
  if (u->user)
    user = u->user;
  else if (opt.user && (opt.use_askpass || opt.ask_passwd))
    user = opt.user;
  else if (opt.ftp_user)
    user = opt.ftp_user;
  else if (opt.user)
    user = opt.user;
  else
    user = NULL;

  /* Find the password with priority */
  if (u->passwd)
    passwd = u->passwd;
  else if (opt.passwd && (opt.use_askpass || opt.ask_passwd))
    passwd = opt.passwd;
  else if (opt.ftp_passwd)
    passwd = opt.ftp_passwd;
  else if (opt.passwd)
    passwd = opt.passwd;
  else
    passwd = NULL;

  /* Check for ~/.netrc if none of the above match */
  if (opt.netrc && (!user || !passwd))
    search_netrc (u->host, (const char **) &user, (const char **) &passwd, 1, NULL);

  if (!user) user = "anonymous";
  if (!passwd) passwd = "-wget@";

  dtsock = -1;
  local_sock = -1;
  con->dltime = 0;

#ifdef HAVE_SSL
  if (u->scheme == SCHEME_FTPS)
    {
      /* Initialize SSL layer first */
      if (!ssl_init ())
        {
          scheme_disable (SCHEME_FTPS);
          logprintf (LOG_NOTQUIET, _("Could not initialize SSL. It will be disabled."));
          err = SSLINITFAILED;
          return err;
        }

      /* If we're using the default FTP port and implicit FTPS was requested,
       * rewrite the port to the default *implicit* FTPS port.
       */
      if (opt.ftps_implicit && u->port == DEFAULT_FTP_PORT)
        {
          DEBUGP (("Implicit FTPS was specified. Rewriting default port to %d.\n", DEFAULT_FTPS_IMPLICIT_PORT));
          u->port = DEFAULT_FTPS_IMPLICIT_PORT;
        }
    }
#endif

  if (!(cmd & DO_LOGIN))
    {
      csock = con->csock;
#ifdef HAVE_SSL
      using_data_security = con->st & DATA_CHANNEL_SECURITY;
#endif
    }
  else                          /* cmd & DO_LOGIN */
    {
      char    *host = con->proxy ? con->proxy->host : u->host;
      int      port = con->proxy ? con->proxy->port : u->port;

      /* Login to the server: */

      /* First: Establish the control connection.  */

      csock = connect_to_host (host, port);
      if (csock == E_HOST)
          return HOSTERR;
      else if (csock < 0)
          return (retryable_socket_connect_error (errno)
                  ? CONERROR : CONIMPOSSIBLE);

      if (cmd & LEAVE_PENDING)
        con->csock = csock;
      else
        con->csock = -1;

#ifdef HAVE_SSL
      if (u->scheme == SCHEME_FTPS)
        {
          /* If we're in implicit FTPS mode, we have to set up SSL/TLS before everything else.
           * Otherwise we first read the server's greeting, and then send an "AUTH TLS".
           */
          if (opt.ftps_implicit)
            {
              err = init_control_ssl_connection (csock, u, &using_control_security);
              if (err != NOCONERROR)
                return err;
              err = get_ftp_greeting (csock, con);
              if (err != FTPOK)
                return err;
            }
          else
            {
              err = get_ftp_greeting (csock, con);
              if (err != FTPOK)
                return err;
              err = init_control_ssl_connection (csock, u, &using_control_security);
              if (err != NOCONERROR)
                return err;
            }
        }
      else
        {
          err = get_ftp_greeting (csock, con);
          if (err != FTPOK)
            return err;
        }
#else
      err = get_ftp_greeting (csock, con);
      if (err != FTPOK)
        return err;
#endif

      /* Second: Login with proper USER/PASS sequence.  */
      logprintf (LOG_VERBOSE, _("Logging in as %s ... "),
                 quotearg_style (escape_quoting_style, user));
      if (opt.server_response)
        logputs (LOG_ALWAYS, "\n");
      if (con->proxy)
        {
          /* If proxy is in use, log in as username@target-site. */
          char *logname = concat_strings (user, "@", u->host, (char *) 0);
          err = ftp_login (csock, logname, passwd);
          xfree (logname);
        }
      else
        err = ftp_login (csock, user, passwd);

      /* FTPRERR, FTPSRVERR, WRITEFAILED, FTPLOGREFUSED, FTPLOGINC */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPSRVERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("Error in server greeting.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case WRITEFAILED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Write failed, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPLOGREFUSED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("The server refuses login.\n"));
          fd_close (csock);
          con->csock = -1;
          return FTPLOGREFUSED;
        case FTPLOGINC:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("Login incorrect.\n"));
          fd_close (csock);
          con->csock = -1;
          return FTPLOGINC;
        case FTPOK:
          if (!opt.server_response)
            logputs (LOG_VERBOSE, _("Logged in!\n"));
          break;
        default:
          abort ();
        }

#ifdef HAVE_SSL
      if (using_control_security)
        {
          /* Send the PBSZ and PROT commands, in that order.
           * If we are here it means that the server has already accepted
           * some form of FTPS. Thus, these commands must work.
           * If they don't work, that's an error. There's no sense in honoring
           * --ftps-fallback-to-ftp or similar options. */
          if (u->scheme == SCHEME_FTPS)
            {
              if (!opt.server_response)
                logputs (LOG_VERBOSE, "==> PBSZ 0 ... ");
              if ((err = ftp_pbsz (csock, 0)) == FTPNOPBSZ)
                {
                  logputs (LOG_NOTQUIET, _("Server did not accept the 'PBSZ 0' command.\n"));
                  return err;
                }
              if (!opt.server_response)
                logputs (LOG_VERBOSE, "done.");

              if (!opt.server_response)
                logprintf (LOG_VERBOSE, "  ==> PROT %c ... ", (int) prot);
              if ((err = ftp_prot (csock, prot)) == FTPNOPROT)
                {
                  logprintf (LOG_NOTQUIET, _("Server did not accept the 'PROT %c' command.\n"), (int) prot);
                  return err;
                }
              if (!opt.server_response)
                logputs (LOG_VERBOSE, "done.\n");

              if (prot != PROT_CLEAR)
                {
                  using_data_security = true;
                  con->st |= DATA_CHANNEL_SECURITY;
                }
            }
        }
#endif

      /* Third: Get the system type */
      if (!opt.server_response)
        logprintf (LOG_VERBOSE, "==> SYST ... ");
      err = ftp_syst (csock, &con->rs, &con->rsu);
      /* FTPRERR */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPSRVERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Server error, can't determine system type.\n"));
          break;
        case FTPOK:
          /* Everything is OK.  */
          break;
        default:
          abort ();
        }
      if (!opt.server_response && err != FTPSRVERR)
        logputs (LOG_VERBOSE, _("done.    "));

      /* 2013-10-17 Andrea Urbani (matfanjol)
         According to the system type I choose which
         list command will be used.
         If I don't know that system, I will try, the
         first time of each session, "LIST -a" and
         "LIST". (see __LIST_A_EXPLANATION__ below) */
      switch (con->rs)
        {
        case ST_VMS:
          /* About ST_VMS there is an old note:
             2008-01-29  SMS.  For a VMS FTP server, where "LIST -a" may not
             fail, but will never do what is desired here,
             skip directly to the simple "LIST" command
             (assumed to be the last one in the list).  */
          DEBUGP (("\nVMS: I know it and I will use \"LIST\" as standard list command\n"));
          con->st |= LIST_AFTER_LIST_A_CHECK_DONE;
          con->st |= AVOID_LIST_A;
          break;
        case ST_UNIX:
          if (con->rsu == UST_MULTINET)
            {
              DEBUGP (("\nUNIX MultiNet: I know it and I will use \"LIST\" "
                       "as standard list command\n"));
              con->st |= LIST_AFTER_LIST_A_CHECK_DONE;
              con->st |= AVOID_LIST_A;
            }
          else if (con->rsu == UST_TYPE_L8)
            {
              DEBUGP (("\nUNIX TYPE L8: I know it and I will use \"LIST -a\" "
                       "as standard list command\n"));
              con->st |= LIST_AFTER_LIST_A_CHECK_DONE;
              con->st |= AVOID_LIST;
            }
          break;
        default:
          break;
        }

      /* Fourth: Find the initial ftp directory */

      if (!opt.server_response)
        logprintf (LOG_VERBOSE, "==> PWD ... ");
      err = ftp_pwd (csock, &con->id);
      /* FTPRERR */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPSRVERR :
          /* PWD unsupported -- assume "/". */
          xfree (con->id);
          con->id = xstrdup ("/");
          break;
        case FTPOK:
          /* Everything is OK.  */
          break;
        default:
          abort ();
        }

#if 0
      /* 2004-09-17 SMS.
         Don't help me out.  Please.
         A reasonably recent VMS FTP server will cope just fine with
         UNIX file specifications.  This code just spoils things.
         Discarding the device name, for example, is not a wise move.
         This code was disabled but left in as an example of what not
         to do.
      */

      /* VMS will report something like "PUB$DEVICE:[INITIAL.FOLDER]".
         Convert it to "/INITIAL/FOLDER" */
      if (con->rs == ST_VMS)
        {
          char *path = strchr (con->id, '[');
          char *pathend = path ? strchr (path + 1, ']') : NULL;
          if (!path || !pathend)
            DEBUGP (("Initial VMS directory not in the form [...]!\n"));
          else
            {
              char *idir = con->id;
              DEBUGP (("Preprocessing the initial VMS directory\n"));
              DEBUGP (("  old = '%s'\n", con->id));
              /* We do the conversion in-place by copying the stuff
                 between [ and ] to the beginning, and changing dots
                 to slashes at the same time.  */
              *idir++ = '/';
              for (++path; path < pathend; path++, idir++)
                *idir = *path == '.' ? '/' : *path;
              *idir = '\0';
              DEBUGP (("  new = '%s'\n\n", con->id));
            }
        }
#endif /* 0 */

      if (!opt.server_response)
        logputs (LOG_VERBOSE, _("done.\n"));

      /* Fifth: Set the FTP type.  */
      type_char = ftp_process_type (u->params);
      if (!opt.server_response)
        logprintf (LOG_VERBOSE, "==> TYPE %c ... ", type_char);
      err = ftp_type (csock, type_char);
      /* FTPRERR, WRITEFAILED, FTPUNKNOWNTYPE */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case WRITEFAILED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Write failed, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPUNKNOWNTYPE:
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET,
                     _("Unknown type `%c', closing control connection.\n"),
                     type_char);
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPOK:
          /* Everything is OK.  */
          break;
        default:
          abort ();
        }
      if (!opt.server_response)
        logputs (LOG_VERBOSE, _("done.  "));
    } /* do login */

  if (cmd & DO_CWD)
    {
      if (!*u->dir)
        logputs (LOG_VERBOSE, _("==> CWD not needed.\n"));
      else
        {
          const char *targ = NULL;
          int cwd_count;
          int cwd_end;
          int cwd_start;

          char *target = u->dir;

          DEBUGP (("changing working directory\n"));

          /* Change working directory.  To change to a non-absolute
             Unix directory, we need to prepend initial directory
             (con->id) to it.  Absolute directories "just work".

             A relative directory is one that does not begin with '/'
             and, on non-Unix OS'es, one that doesn't begin with
             "[a-z]:".

             This is not done for OS400, which doesn't use
             "/"-delimited directories, nor does it support directory
             hierarchies.  "CWD foo" followed by "CWD bar" leaves us
             in "bar", not in "foo/bar", as would be customary
             elsewhere.  */

            /* 2004-09-20 SMS.
               Why is this wise even on UNIX?  It certainly fouls VMS.
               See below for a more reliable, more universal method.
            */

            /* 2008-04-22 MJC.
               I'm not crazy about it either. I'm informed it's useful
               for misconfigured servers that have some dirs in the path
               with +x but -r, but this method is not RFC-conformant. I
               understand the need to deal with crappy server
               configurations, but it's far better to use the canonical
               method first, and fall back to kludges second.
            */

          if (target[0] != '/'
              && !(con->rs != ST_UNIX
                   && c_isalpha (target[0])
                   && target[1] == ':')
              && (con->rs != ST_OS400)
              && (con->rs != ST_VMS))
            {
              int idlen = strlen (con->id);
              char *ntarget, *p;

              /* Strip trailing slash(es) from con->id. */
              while (idlen > 0 && con->id[idlen - 1] == '/')
                --idlen;
              p = ntarget = (char *)alloca (idlen + 1 + strlen (u->dir) + 1);
              memcpy (p, con->id, idlen);
              p += idlen;
              *p++ = '/';
              strcpy (p, target);

              DEBUGP (("Prepended initial PWD to relative path:\n"));
              DEBUGP (("   pwd: '%s'\n   old: '%s'\n  new: '%s'\n",
                       con->id, target, ntarget));
              target = ntarget;
            }

#if 0
          /* 2004-09-17 SMS.
             Don't help me out.  Please.
             A reasonably recent VMS FTP server will cope just fine with
             UNIX file specifications.  This code just spoils things.
             Discarding the device name, for example, is not a wise
             move.
             This code was disabled but left in as an example of what
             not to do.
          */

          /* If the FTP host runs VMS, we will have to convert the absolute
             directory path in UNIX notation to absolute directory path in
             VMS notation as VMS FTP servers do not like UNIX notation of
             absolute paths.  "VMS notation" is [dir.subdir.subsubdir]. */

          if (con->rs == ST_VMS)
            {
              char *tmpp;
              char *ntarget = (char *)alloca (strlen (target) + 2);
              /* We use a converted initial dir, so directories in
                 TARGET will be separated with slashes, something like
                 "/INITIAL/FOLDER/DIR/SUBDIR".  Convert that to
                 "[INITIAL.FOLDER.DIR.SUBDIR]".  */
              strcpy (ntarget, target);
              assert (*ntarget == '/');
              *ntarget = '[';
              for (tmpp = ntarget + 1; *tmpp; tmpp++)
                if (*tmpp == '/')
                  *tmpp = '.';
              *tmpp++ = ']';
              *tmpp = '\0';
              DEBUGP (("Changed file name to VMS syntax:\n"));
              DEBUGP (("  Unix: '%s'\n  VMS: '%s'\n", target, ntarget));
              target = ntarget;
            }
#endif /* 0 */

          /* 2004-09-20 SMS.
             A relative directory is relative to the initial directory.
             Thus, what _is_ useful on VMS (and probably elsewhere) is
             to CWD to the initial directory (ideally, whatever the
             server reports, _exactly_, NOT badly UNIX-ixed), and then
             CWD to the (new) relative directory.  This should probably
             be restructured as a function, called once or twice, but
             I'm lazy enough to take the badly indented loop short-cut
             for now.
          */

          /* Decide on one pass (absolute) or two (relative).
             The VMS restriction may be relaxed when the squirrely code
             above is reformed.
          */
          if ((con->rs == ST_VMS) && (target[0] != '/'))
            {
              cwd_start = 0;
              DEBUGP (("Using two-step CWD for relative path.\n"));
            }
          else
            {
              /* Go straight to the target. */
              cwd_start = 1;
            }

          /* At least one VMS FTP server (TCPware V5.6-2) can switch to
             a UNIX emulation mode when given a UNIX-like directory
             specification (like "a/b/c").  If allowed to continue this
             way, LIST interpretation will be confused, because the
             system type (SYST response) will not be re-checked, and
             future UNIX-format directory listings (for multiple URLs or
             "-r") will be horribly misinterpreted.

             The cheap and nasty work-around is to do a "CWD []" after a
             UNIX-like directory specification is used.  (A single-level
             directory is harmless.)  This puts the TCPware server back
             into VMS mode, and does no harm on other servers.

             Unlike the rest of this block, this particular behavior
             _is_ VMS-specific, so it gets its own VMS test.
          */
          if ((con->rs == ST_VMS) && (strchr (target, '/') != NULL))
            {
              cwd_end = 3;
              DEBUGP (("Using extra \"CWD []\" step for VMS server.\n"));
            }
          else
            {
              cwd_end = 2;
            }

          /* 2004-09-20 SMS. */
          /* Sorry about the deviant indenting.  Laziness. */

          for (cwd_count = cwd_start; cwd_count < cwd_end; cwd_count++)
            {
              switch (cwd_count)
                {
                  case 0:
                    /* Step one (optional): Go to the initial directory,
                       exactly as reported by the server.
                    */
                    targ = con->id;
                    break;

                  case 1:
                    /* Step two: Go to the target directory.  (Absolute or
                       relative will work now.)
                    */
                    targ = target;
                    break;

                  case 2:
                    /* Step three (optional): "CWD []" to restore server
                       VMS-ness.
                    */
                    targ = "[]";
                    break;

                  default:
                    logprintf (LOG_ALWAYS, _("Logically impossible section reached in getftp()"));
                    logprintf (LOG_ALWAYS, _("cwd_count: %d\ncwd_start: %d\ncwd_end: %d\n"),
                                             cwd_count, cwd_start, cwd_end);
                    abort ();
                }

              if (!opt.server_response)
                logprintf (LOG_VERBOSE, "==> CWD (%d) %s ... ", cwd_count,
                           quotearg_style (escape_quoting_style, target));

              err = ftp_cwd (csock, targ);

              /* FTPRERR, WRITEFAILED, FTPNSFOD */
              switch (err)
                {
                  case FTPRERR:
                    logputs (LOG_VERBOSE, "\n");
                    logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
                    fd_close (csock);
                    con->csock = -1;
                    return err;
                  case WRITEFAILED:
                    logputs (LOG_VERBOSE, "\n");
                    logputs (LOG_NOTQUIET,
                             _("Write failed, closing control connection.\n"));
                    fd_close (csock);
                    con->csock = -1;
                    return err;
                  case FTPNSFOD:
                    logputs (LOG_VERBOSE, "\n");
                    logprintf (LOG_NOTQUIET, _("No such directory %s.\n\n"),
                               quote (u->dir));
                    fd_close (csock);
                    con->csock = -1;
                    return err;
                  case FTPOK:
                    break;
                  default:
                    abort ();
                }

              if (!opt.server_response)
                logputs (LOG_VERBOSE, _("done.\n"));

            } /* for */

          /* 2004-09-20 SMS. */

        } /* else */
    }
  else /* do not CWD */
    logputs (LOG_VERBOSE, _("==> CWD not required.\n"));

  if ((cmd & DO_RETR) && passed_expected_bytes == 0)
    {
      if (opt.verbose)
        {
          if (!opt.server_response)
            logprintf (LOG_VERBOSE, "==> SIZE %s ... ",
                       quotearg_style (escape_quoting_style, u->file));
        }

      err = ftp_size (csock, u->file, &expected_bytes);
      /* FTPRERR */
      switch (err)
        {
        case FTPRERR:
        case FTPSRVERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          return err;
        case FTPOK:
          got_expected_bytes = true;
          /* Everything is OK.  */
          break;
        default:
          abort ();
        }
        if (!opt.server_response)
          {
            logprintf (LOG_VERBOSE, "%s\n",
                    expected_bytes ?
                    number_to_static_string (expected_bytes) :
                    _("done.\n"));
          }
    }

  if (cmd & DO_RETR && restval > 0 && restval == expected_bytes)
    {
      /* Server confirms that file has length restval. We should stop now.
         Some servers (f.e. NcFTPd) return error when receive REST 0 */
      logputs (LOG_VERBOSE, _("File has already been retrieved.\n"));
      fd_close (csock);
      con->csock = -1;
      return RETRFINISHED;
    }

  do
  {
  try_again = false;
  /* If anything is to be retrieved, PORT (or PASV) must be sent.  */
  if (cmd & (DO_LIST | DO_RETR))
    {
      if (opt.ftp_pasv)
        {
          ip_address passive_addr;
          int        passive_port;
          err = ftp_do_pasv (csock, &passive_addr, &passive_port);
          /* FTPRERR, WRITEFAILED, FTPNOPASV, FTPINVPASV */
          switch (err)
            {
            case FTPRERR:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
              fd_close (csock);
              con->csock = -1;
              return err;
            case WRITEFAILED:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET,
                       _("Write failed, closing control connection.\n"));
              fd_close (csock);
              con->csock = -1;
              return err;
            case FTPNOPASV:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("Cannot initiate PASV transfer.\n"));
              break;
            case FTPINVPASV:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("Cannot parse PASV response.\n"));
              break;
            case FTPOK:
              break;
            default:
              abort ();
            }   /* switch (err) */
          if (err==FTPOK)
            {
              DEBUGP (("trying to connect to %s port %d\n",
                      print_address (&passive_addr), passive_port));
              dtsock = connect_to_ip (&passive_addr, passive_port, NULL);
              if (dtsock < 0)
                {
                  int save_errno = errno;
                  fd_close (csock);
                  con->csock = -1;
                  logprintf (LOG_VERBOSE, _("couldn't connect to %s port %d: %s\n"),
                             print_address (&passive_addr), passive_port,
                             strerror (save_errno));
                  return (retryable_socket_connect_error (save_errno)
                          ? CONERROR : CONIMPOSSIBLE);
                }

              if (!opt.server_response)
                logputs (LOG_VERBOSE, _("done.    "));
            }
          else
            return err;

          /*
           * We do not want to fall back from PASSIVE mode to ACTIVE mode !
           * The reason is the PORT command exposes the client's real IP address
           * to the server. Bad for someone who relies on privacy via a ftp proxy.
           */
        }
      else
        {
          err = ftp_do_port (csock, &local_sock);
          /* FTPRERR, WRITEFAILED, bindport (FTPSYSERR), HOSTERR,
             FTPPORTERR */
          switch (err)
            {
            case FTPRERR:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
              fd_close (csock);
              con->csock = -1;
              fd_close (dtsock);
              fd_close (local_sock);
              return err;
            case WRITEFAILED:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET,
                       _("Write failed, closing control connection.\n"));
              fd_close (csock);
              con->csock = -1;
              fd_close (dtsock);
              fd_close (local_sock);
              return err;
            case CONSOCKERR:
              logputs (LOG_VERBOSE, "\n");
              logprintf (LOG_NOTQUIET, "socket: %s\n", strerror (errno));
              fd_close (csock);
              con->csock = -1;
              fd_close (dtsock);
              fd_close (local_sock);
              return err;
            case FTPSYSERR:
              logputs (LOG_VERBOSE, "\n");
              logprintf (LOG_NOTQUIET, _("Bind error (%s).\n"),
                         strerror (errno));
              fd_close (dtsock);
              return err;
            case FTPPORTERR:
              logputs (LOG_VERBOSE, "\n");
              logputs (LOG_NOTQUIET, _("Invalid PORT.\n"));
              fd_close (csock);
              con->csock = -1;
              fd_close (dtsock);
              fd_close (local_sock);
              return err;
            case FTPOK:
              break;
            default:
              abort ();
            } /* port switch */
          if (!opt.server_response)
            logputs (LOG_VERBOSE, _("done.    "));
        } /* dtsock == -1 */
    } /* cmd & (DO_LIST | DO_RETR) */

  /* Restart if needed.  */
  if (restval && (cmd & DO_RETR))
    {
      if (!opt.server_response)
        logprintf (LOG_VERBOSE, "==> REST %s ... ",
                   number_to_static_string (restval));
      err = ftp_rest (csock, restval);

      /* FTPRERR, WRITEFAILED, FTPRESTFAIL */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case WRITEFAILED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Write failed, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case FTPRESTFAIL:
          logputs (LOG_VERBOSE, _("\nREST failed, starting from scratch.\n"));
          rest_failed = true;
          break;
        case FTPOK:
          break;
        default:
          abort ();
        }
      if (err != FTPRESTFAIL && !opt.server_response)
        logputs (LOG_VERBOSE, _("done.    "));
    } /* restval && cmd & DO_RETR */

  if (cmd & DO_RETR)
    {
      /* If we're in spider mode, don't really retrieve anything except
         the directory listing and verify whether the given "file" exists.  */
      if (opt.spider)
        {
          bool exists = false;
          bool all_exist = true;
          struct fileinfo *f;
          uerr_t _res = ftp_get_listing (u, original_url, con, &f);
          /* Set the DO_RETR command flag again, because it gets unset when
             calling ftp_get_listing() and would otherwise cause an assertion
             failure earlier on when this function gets repeatedly called
             (e.g., when recursing).  */
          con->cmd |= DO_RETR;
          if (_res == RETROK)
            {
              while (f)
                {
                  if (!strcmp (f->name, u->file))
                    {
                      exists = true;
                      break;
                    } else {
                      all_exist = false;
                    }
                  f = f->next;
                }
              if (exists)
                {
                  logputs (LOG_VERBOSE, "\n");
                  logprintf (LOG_NOTQUIET, _("File %s exists.\n"),
                             quote (u->file));
                }
              else
                {
                  logputs (LOG_VERBOSE, "\n");
                  logprintf (LOG_NOTQUIET, _("No such file %s.\n"),
                             quote (u->file));
                }
            }
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          if (all_exist) {
              return RETRFINISHED;
          } else {
              return FTPNSFOD;
          }
        }

      if (opt.verbose)
        {
          if (!opt.server_response)
            {
              if (restval)
                logputs (LOG_VERBOSE, "\n");
              logprintf (LOG_VERBOSE, "==> RETR %s ... ",
                         quotearg_style (escape_quoting_style, u->file));
            }
        }

      err = ftp_retr (csock, u->file);
      /* FTPRERR, WRITEFAILED, FTPNSFOD */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case WRITEFAILED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Write failed, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case FTPNSFOD:
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("No such file %s.\n\n"),
                     quote (u->file));
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case FTPOK:
          break;
        default:
          abort ();
        }

      if (!opt.server_response)
        logputs (LOG_VERBOSE, _("done.\n"));

      if (! got_expected_bytes)
        expected_bytes = *last_expected_bytes;
    } /* do retrieve */

  if (cmd & DO_LIST)
    {
      if (!opt.server_response)
        logputs (LOG_VERBOSE, "==> LIST ... ");
      /* As Maciej W. Rozycki (macro@ds2.pg.gda.pl) says, `LIST'
         without arguments is better than `LIST .'; confirmed by
         RFC959.  */
      err = ftp_list (csock, NULL, con->st&AVOID_LIST_A, con->st&AVOID_LIST, &list_a_used);

      /* FTPRERR, WRITEFAILED */
      switch (err)
        {
        case FTPRERR:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET, _("\
Error in server response, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case WRITEFAILED:
          logputs (LOG_VERBOSE, "\n");
          logputs (LOG_NOTQUIET,
                   _("Write failed, closing control connection.\n"));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case FTPNSFOD:
          logputs (LOG_VERBOSE, "\n");
          logprintf (LOG_NOTQUIET, _("No such file or directory %s.\n\n"),
                     quote ("."));
          fd_close (dtsock);
          fd_close (local_sock);
          return err;
        case FTPOK:
          break;
        default:
          abort ();
        }
      if (!opt.server_response)
        logputs (LOG_VERBOSE, _("done.\n"));

      if (! got_expected_bytes)
        expected_bytes = *last_expected_bytes;
    } /* cmd & DO_LIST */

  if (!(cmd & (DO_LIST | DO_RETR)) || (opt.spider && !(cmd & DO_LIST)))
    return RETRFINISHED;

  /* Some FTP servers return the total length of file after REST
     command, others just return the remaining size. */
  if (passed_expected_bytes && restval && expected_bytes
      && (expected_bytes == passed_expected_bytes - restval))
    {
      DEBUGP (("Lying FTP server found, adjusting.\n"));
      expected_bytes = passed_expected_bytes;
    }

  /* If no transmission was required, then everything is OK.  */
  if (!opt.ftp_pasv)  /* we are not using passive mode so we need
                         to accept */
    {
      /* Wait for the server to connect to the address we're waiting
         at.  */
      dtsock = accept_connection (local_sock);
      if (dtsock < 0)
        {
          logprintf (LOG_NOTQUIET, "accept: %s\n", strerror (errno));
          return CONERROR;
        }
    }

  /* Open the file -- if output_stream is set, use it instead.  */

  /* 2005-04-17 SMS.
     Note that having the output_stream ("-O") file opened in main
     (main.c) rather limits the ability in VMS to open the file
     differently for ASCII versus binary FTP here.  (Of course, doing it
     there allows a open failure to be detected immediately, without first
     connecting to the server.)
  */
  if (!output_stream || con->cmd & DO_LIST)
    {
/* On VMS, alter the name as required. */
#ifdef __VMS
      char *targ;

      targ = ods_conform (con->target);
      if (targ != con->target)
        {
          xfree (con->target);
          con->target = targ;
        }
#endif /* def __VMS */

      mkalldirs (con->target);
      if (opt.backups)
        rotate_backups (con->target);

/* 2005-04-15 SMS.
   For VMS, define common fopen() optional arguments, and a handy macro
   for use as a variable "binary" flag.
   Elsewhere, define a constant "binary" flag.
   Isn't it nice to have distinct text and binary file types?
*/
/* 2011-09-30 SMS.
   Added listing files to the set of non-"binary" (text, Stream_LF)
   files.  (Wget works either way, but other programs, like, say, text
   editors, work better on listing files which have text attributes.)
   Now we use "binary" attributes for a binary ("IMAGE") transfer,
   unless "--ftp-stmlf" was specified, and we always use non-"binary"
   (text, Stream_LF) attributes for a listing file, or for an ASCII
   transfer.
   Tidied the VMS-specific BIN_TYPE_xxx macros, and changed the call to
   fopen_excl() (restored?) to use BIN_TYPE_FILE instead of "true".
*/
#ifdef __VMS
# define BIN_TYPE_TRANSFER (type_char != 'A')
# define BIN_TYPE_FILE \
   ((!(cmd & DO_LIST)) && BIN_TYPE_TRANSFER && (opt.ftp_stmlf == 0))
# define FOPEN_OPT_ARGS "fop=sqo", "acc", acc_cb, &open_id
# define FOPEN_OPT_ARGS_BIN "ctx=bin,stm", "rfm=fix", "mrs=512" FOPEN_OPT_ARGS
#else /* def __VMS */
# define BIN_TYPE_FILE true
#endif /* def __VMS [else] */

      if (restval && !(con->cmd & DO_LIST))
        {
#ifdef __VMS
          int open_id;

          if (BIN_TYPE_FILE)
            {
              open_id = 3;
              fp = fopen (con->target, "ab", FOPEN_OPT_ARGS_BIN);
            }
          else
            {
              open_id = 4;
              fp = fopen (con->target, "a", FOPEN_OPT_ARGS);
            }
#else /* def __VMS */
          fp = fopen (con->target, "ab");
#endif /* def __VMS [else] */
        }
      else if (opt.noclobber || opt.always_rest || opt.timestamping || opt.dirstruct
               || opt.output_document || count > 0)
        {
          if (opt.unlink_requested && file_exists_p (con->target, NULL))
            {
              if (unlink (con->target) < 0)
                {
                  logprintf (LOG_NOTQUIET, "%s: %s\n", con->target,
                    strerror (errno));
                    fd_close (csock);
                    con->csock = -1;
                    fd_close (dtsock);
                    fd_close (local_sock);
                    return UNLINKERR;
                }
            }

#ifdef __VMS
          int open_id;

          if (BIN_TYPE_FILE)
            {
              open_id = 5;
              fp = fopen (con->target, "wb", FOPEN_OPT_ARGS_BIN);
            }
          else
            {
              open_id = 6;
              fp = fopen (con->target, "w", FOPEN_OPT_ARGS);
            }
#else /* def __VMS */
          fp = fopen (con->target, "wb");
#endif /* def __VMS [else] */
        }
      else
        {
          fp = fopen_excl (con->target, BIN_TYPE_FILE);
          if (!fp && errno == EEXIST)
            {
              /* We cannot just invent a new name and use it (which is
                 what functions like unique_create typically do)
                 because we told the user we'd use this name.
                 Instead, return and retry the download.  */
              logprintf (LOG_NOTQUIET, _("%s has sprung into existence.\n"),
                         con->target);
              fd_close (csock);
              con->csock = -1;
              fd_close (dtsock);
              fd_close (local_sock);
              return FOPEN_EXCL_ERR;
            }
        }
      if (!fp)
        {
          logprintf (LOG_NOTQUIET, "%s: %s\n", con->target, strerror (errno));
          fd_close (csock);
          con->csock = -1;
          fd_close (dtsock);
          fd_close (local_sock);
          return FOPENERR;
        }
    }
  else
    fp = output_stream;

  if (passed_expected_bytes)
    {
      print_length (passed_expected_bytes, restval, true);
      expected_bytes = passed_expected_bytes;
        /* for fd_read_body's progress bar */
    }
  else if (expected_bytes)
    print_length (expected_bytes, restval, false);

#ifdef HAVE_SSL
  if (u->scheme == SCHEME_FTPS && using_data_security)
    {
      /* We should try to restore the existing SSL session in the data connection
       * and fall back to establishing a new session if the server doesn't want to restore it.
       */
      if (!opt.ftps_resume_ssl || !ssl_connect_wget (dtsock, u->host, &csock))
        {
          if (opt.ftps_resume_ssl)
            logputs (LOG_NOTQUIET, "Server does not want to resume the SSL session. Trying with a new one.\n");
          if (!ssl_connect_wget (dtsock, u->host, NULL))
            {
              fd_close (csock);
              fd_close (dtsock);
              err = CONERROR;
              logputs (LOG_NOTQUIET, "Could not perform SSL handshake.\n");
              goto exit_error;
            }
        }
      else
        logputs (LOG_NOTQUIET, "Resuming SSL session in data connection.\n");

      if (!ssl_check_certificate (dtsock, u->host))
        {
          fd_close (csock);
          fd_close (dtsock);
          err = CONERROR;
          goto exit_error;
        }
    }
#endif

  /* Get the contents of the document.  */
  flags = 0;
  if (restval && rest_failed)
    flags |= rb_skip_startpos;
  rd_size = 0;
  res = fd_read_body (con->target, dtsock, fp,
                      expected_bytes ? expected_bytes - restval : 0,
                      restval, &rd_size, qtyread, &con->dltime, flags, warc_tmp);

  tms = datetime_str (time (NULL));
  tmrate = retr_rate (rd_size, con->dltime);
  total_download_time += con->dltime;

#ifdef ENABLE_XATTR
  if (opt.enable_xattr)
    set_file_metadata (u->url, NULL, fp);
#endif

  fd_close (local_sock);
  /* Close the local file.  */
  if (!output_stream || con->cmd & DO_LIST)
    fclose (fp);

  /* If fd_read_body couldn't write to fp or warc_tmp, bail out.  */
  if (res == -2 || (warc_tmp != NULL && res == -3))
    {
      logprintf (LOG_NOTQUIET, _("%s: %s, closing control connection.\n"),
                 con->target, strerror (errno));
      fd_close (csock);
      con->csock = -1;
      fd_close (dtsock);
      if (res == -2)
        return FWRITEERR;
      else if (res == -3)
        return WARC_TMP_FWRITEERR;
    }
  else if (res == -1)
    {
      logprintf (LOG_NOTQUIET, _("%s (%s) - Data connection: %s; "),
                 tms, tmrate, fd_errstr (dtsock));
      if (opt.server_response)
        logputs (LOG_ALWAYS, "\n");
    }
  fd_close (dtsock);

  /* Get the server to tell us if everything is retrieved.  */
  err = ftp_response (csock, &respline);
  if (err != FTPOK)
    {
      /* The control connection is decidedly closed.  Print the time
         only if it hasn't already been printed.  */
      if (res != -1)
        logprintf (LOG_NOTQUIET, "%s (%s) - ", tms, tmrate);
      logputs (LOG_NOTQUIET, _("Control connection closed.\n"));
      /* If there is an error on the control connection, close it, but
         return FTPRETRINT, since there is a possibility that the
         whole file was retrieved nevertheless (but that is for
         ftp_loop_internal to decide).  */
      fd_close (csock);
      con->csock = -1;
      return FTPRETRINT;
    } /* err != FTPOK */
  *last_expected_bytes = ftp_expected_bytes (respline);
  /* If retrieval failed for any reason, return FTPRETRINT, but do not
     close socket, since the control connection is still alive.  If
     there is something wrong with the control connection, it will
     become apparent later.  */
  if (*respline != '2')
    {
      if (res != -1)
        logprintf (LOG_NOTQUIET, "%s (%s) - ", tms, tmrate);
      logputs (LOG_NOTQUIET, _("Data transfer aborted.\n"));
#ifdef HAVE_SSL
      if (!c_strncasecmp (respline, "425", 3) && u->scheme == SCHEME_FTPS)
        {
          logputs (LOG_NOTQUIET, "FTPS server rejects new SSL sessions in the data connection.\n");
          xfree (respline);
          return FTPRESTFAIL;
        }
#endif
      xfree (respline);
      return FTPRETRINT;
    }
  xfree (respline);

  if (res == -1)
    {
      /* What now?  The data connection was erroneous, whereas the
         response says everything is OK.  We shall play it safe.  */
      return FTPRETRINT;
    }

  if (!(cmd & LEAVE_PENDING))
    {
      /* Closing the socket is faster than sending 'QUIT' and the
         effect is the same.  */
      fd_close (csock);
      con->csock = -1;
    }
  /* If it was a listing, and opt.server_response is true,
     print it out.  */
  if (con->cmd & DO_LIST)
    {
      if (opt.server_response)
        {
/* 2005-02-25 SMS.
   Much of this work may already have been done, but repeating it should
   do no damage beyond wasting time.
*/
/* On VMS, alter the name as required. */
#ifdef __VMS
      char *targ;

      targ = ods_conform (con->target);
      if (targ != con->target)
        {
          xfree (con->target);
          con->target = targ;
        }
#endif /* def __VMS */

      mkalldirs (con->target);
      fp = fopen (con->target, "r");
      if (!fp)
        logprintf (LOG_ALWAYS, "%s: %s\n", con->target, strerror (errno));
      else
        {
          char *line = NULL;
          size_t bufsize = 0;
          ssize_t len;

          /* The lines are being read with getline because of
             no-buffering on opt.lfile.  */
          while ((len = getline (&line, &bufsize, fp)) > 0)
            {
              while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
                line[--len] = '\0';
              logprintf (LOG_ALWAYS, "%s\n",
                         quotearg_style (escape_quoting_style, line));
            }
          xfree (line);
          fclose (fp);
        }
        } /* server_response */

      /* 2013-10-17 Andrea Urbani (matfanjol)
         < __LIST_A_EXPLANATION__ >
          After the SYST command, looks if it knows that system.
          If yes, wget will force the use of "LIST" or "LIST -a".
          If no, wget will try, only the first time of each session, before the
          "LIST -a" command and after the "LIST".
          If "LIST -a" works and returns more or equal data of the "LIST",
          "LIST -a" will be the standard list command for all the session.
          If "LIST -a" fails or returns less data than "LIST" (think on the case
          of an existing file called "-a"), "LIST" will be the standard list
          command for all the session.
          ("LIST -a" is used to get also the hidden files)

          */
      if (!(con->st & LIST_AFTER_LIST_A_CHECK_DONE))
        {
          /* We still have to check "LIST" after the first "LIST -a" to see
             if with "LIST" we get more data than "LIST -a", that means
             "LIST -a" returned files/folders with "-a" name. */
          if (con->st & AVOID_LIST_A)
            {
              /* LIST was used in this cycle.
                 Let's see the result. */
              if (rd_size > previous_rd_size)
                {
                  /* LIST returns more data than "LIST -a".
                     "LIST" is the official command to use. */
                  con->st |= LIST_AFTER_LIST_A_CHECK_DONE;
                  DEBUGP (("LIST returned more data than \"LIST -a\": "
                           "I will use \"LIST\" as standard list command\n"));
                }
              else if (previous_rd_size > rd_size)
                {
                  /* "LIST -a" returned more data then LIST.
                     "LIST -a" is the official command to use. */
                  con->st |= LIST_AFTER_LIST_A_CHECK_DONE;
                  con->st |= AVOID_LIST;
                  con->st &= ~AVOID_LIST_A;
                  /* Sorry, please, download again the "LIST -a"... */
                  try_again = true;
                  DEBUGP (("LIST returned less data than \"LIST -a\": I will "
                           "use \"LIST -a\" as standard list command\n"));
                }
              else
                {
                  /* LIST and "LIST -a" return the same data. */
                  if (rd_size == 0)
                    {
                      /* Same empty data. We will check both again because
                         we cannot check if "LIST -a" has returned an empty
                         folder instead of a folder content. */
                      con->st &= ~AVOID_LIST_A;
                    }
                  else
                    {
                      /* Same data, so, better to take "LIST -a" that
                         shows also hidden files/folders (when present) */
                      con->st |= LIST_AFTER_LIST_A_CHECK_DONE;
                      con->st |= AVOID_LIST;
                      con->st &= ~AVOID_LIST_A;
                      DEBUGP (("LIST returned the same amount of data of "
                               "\"LIST -a\": I will use \"LIST -a\" as standard "
                               "list command\n"));
                    }
                }
            }
          else
            {
              /* In this cycle "LIST -a" should being used. Is it true? */
              if (list_a_used)
                {
                  /* Yes, it is.
                     OK, let's save the amount of data and try again
                     with LIST */
                  previous_rd_size = rd_size;
                  try_again = true;
                  con->st |= AVOID_LIST_A;
                }
              else
                {
                  /* No: something happens and LIST was used.
                     This means "LIST -a" raises an error. */
                  con->st |= LIST_AFTER_LIST_A_CHECK_DONE;
                  con->st |= AVOID_LIST_A;
                  DEBUGP (("\"LIST -a\" failed: I will use \"LIST\" "
                           "as standard list command\n"));
                }
            }
        }
    }
  } while (try_again);
  return RETRFINISHED;

exit_error:

  /* If fp is a regular file, close and try to remove it */
  if (fp && (!output_stream || con->cmd & DO_LIST))
    fclose (fp);
  return err;
}