void mutt_init (int skip_sys_rc, LIST *commands)
{
  struct passwd *pw;
  struct utsname utsname;
  char *p, buffer[STRING], error[STRING];
  int i, default_rc = 0, need_pause = 0;
  BUFFER err;

  memset (&err, 0, sizeof (err));
  err.data = error;
  err.dsize = sizeof (error);

  Groups = hash_create (1031, 0);
  ReverseAlias = hash_create (1031, 1);
  
  mutt_menu_init ();

  /* 
   * XXX - use something even more difficult to predict?
   */
  snprintf (AttachmentMarker, sizeof (AttachmentMarker),
	    "\033]9;%ld\a", (long) time (NULL));
  
  /* on one of the systems I use, getcwd() does not return the same prefix
     as is listed in the passwd file */
  if ((p = getenv ("HOME")))
    Homedir = safe_strdup (p);

  /* Get some information about the user */
  if ((pw = getpwuid (getuid ())))
  {
    char rnbuf[STRING];

    Username = safe_strdup (pw->pw_name);
    if (!Homedir)
      Homedir = safe_strdup (pw->pw_dir);

    Realname = safe_strdup (mutt_gecos_name (rnbuf, sizeof (rnbuf), pw));
    Shell = safe_strdup (pw->pw_shell);
    endpwent ();
  }
  else 
  {
    if (!Homedir)
    {
      mutt_endwin (NULL);
      fputs (_("unable to determine home directory"), stderr);
      exit (1);
    }
    if ((p = getenv ("USER")))
      Username = safe_strdup (p);
    else
    {
      mutt_endwin (NULL);
      fputs (_("unable to determine username"), stderr);
      exit (1);
    }
    Shell = safe_strdup ((p = getenv ("SHELL")) ? p : "/bin/sh");
  }

#ifdef DEBUG
  /* Start up debugging mode if requested */
  if (debuglevel > 0)
    start_debug ();
#endif

  /* And about the host... */
  uname (&utsname);
  /* some systems report the FQDN instead of just the hostname */
  if ((p = strchr (utsname.nodename, '.')))
  {
    Hostname = mutt_substrdup (utsname.nodename, p);
    p++;
    strfcpy (buffer, p, sizeof (buffer)); /* save the domain for below */
  }
  else
    Hostname = safe_strdup (utsname.nodename);

#ifndef DOMAIN
#define DOMAIN buffer
  if (!p && getdnsdomainname (buffer, sizeof (buffer)) == -1)
    Fqdn = safe_strdup ("@");
  else
#endif /* DOMAIN */
    if (*DOMAIN != '@')
  {
    Fqdn = safe_malloc (mutt_strlen (DOMAIN) + mutt_strlen (Hostname) + 2);
    sprintf (Fqdn, "%s.%s", NONULL(Hostname), DOMAIN);	/* __SPRINTF_CHECKED__ */
  }
  else
    Fqdn = safe_strdup(NONULL(Hostname));

  if ((p = getenv ("MAIL")))
    Spoolfile = safe_strdup (p);
  else if ((p = getenv ("MAILDIR")))
    Spoolfile = safe_strdup (p);
  else
  {
#ifdef HOMESPOOL
    mutt_concat_path (buffer, NONULL (Homedir), MAILPATH, sizeof (buffer));
#else
    mutt_concat_path (buffer, MAILPATH, NONULL(Username), sizeof (buffer));
#endif
    Spoolfile = safe_strdup (buffer);
  }

  if ((p = getenv ("MAILCAPS")))
    MailcapPath = safe_strdup (p);
  else
  {
    /* Default search path from RFC1524 */
    MailcapPath = safe_strdup ("~/.mailcap:" PKGDATADIR "/mailcap:" SYSCONFDIR "/mailcap:/etc/mailcap:/usr/etc/mailcap:/usr/local/etc/mailcap");
  }

  Tempdir = safe_strdup ((p = getenv ("TMPDIR")) ? p : "/tmp");

  p = getenv ("VISUAL");
  if (!p)
  {
    p = getenv ("EDITOR");
    if (!p)
      p = "vi";
  }
  Editor = safe_strdup (p);
  Visual = safe_strdup (p);

  if ((p = getenv ("REPLYTO")) != NULL)
  {
    BUFFER buf, token;

    snprintf (buffer, sizeof (buffer), "Reply-To: %s", p);

    memset (&buf, 0, sizeof (buf));
    buf.data = buf.dptr = buffer;
    buf.dsize = mutt_strlen (buffer);

    memset (&token, 0, sizeof (token));
    parse_my_hdr (&token, &buf, 0, &err);
    FREE (&token.data);
  }

  if ((p = getenv ("EMAIL")) != NULL)
    From = rfc822_parse_adrlist (NULL, p);
  
  mutt_set_langinfo_charset ();
  mutt_set_charset (Charset);
  
  Matches = safe_calloc (Matches_listsize, sizeof (char *));
  
  /* Set standard defaults */
  for (i = 0; MuttVars[i].option; i++)
  {
    mutt_set_default (&MuttVars[i]);
    mutt_restore_default (&MuttVars[i]);
  }

  CurrentMenu = MENU_MAIN;


#ifndef LOCALES_HACK
  /* Do we have a locale definition? */
  if (((p = getenv ("LC_ALL")) != NULL && p[0]) ||
      ((p = getenv ("LANG")) != NULL && p[0]) ||
      ((p = getenv ("LC_CTYPE")) != NULL && p[0]))
    set_option (OPTLOCALES);
#endif

#ifdef HAVE_GETSID
  /* Unset suspend by default if we're the session leader */
  if (getsid(0) == getpid())
    unset_option (OPTSUSPEND);
#endif

  mutt_init_history ();

  
  
  
  /*
   * 
   *			   BIG FAT WARNING
   * 
   * When changing the code which looks for a configuration file,
   * please also change the corresponding code in muttbug.sh.in.
   * 
   * 
   */
  
  
  
  
  if (!Muttrc)
  {
    snprintf (buffer, sizeof(buffer), "%s/.muttrc-%s", NONULL(Homedir), MUTT_VERSION);
    if (access(buffer, F_OK) == -1)
      snprintf (buffer, sizeof(buffer), "%s/.muttrc", NONULL(Homedir));
    if (access(buffer, F_OK) == -1)
      snprintf (buffer, sizeof (buffer), "%s/.mutt/muttrc-%s", NONULL(Homedir), MUTT_VERSION);
    if (access(buffer, F_OK) == -1)
      snprintf (buffer, sizeof (buffer), "%s/.mutt/muttrc", NONULL(Homedir));
    if (access(buffer, F_OK) == -1) /* default to .muttrc for alias_file */
      snprintf (buffer, sizeof(buffer), "%s/.muttrc", NONULL(Homedir));

    default_rc = 1;
    Muttrc = safe_strdup (buffer);
  }
  else
  {
    strfcpy (buffer, Muttrc, sizeof (buffer));
    FREE (&Muttrc);
    mutt_expand_path (buffer, sizeof (buffer));
    Muttrc = safe_strdup (buffer);
  }
  FREE (&AliasFile);
  AliasFile = safe_strdup (NONULL(Muttrc));

  /* Process the global rc file if it exists and the user hasn't explicity
     requested not to via "-n".  */
  if (!skip_sys_rc)
  {
    snprintf (buffer, sizeof(buffer), "%s/Muttrc-%s", SYSCONFDIR, MUTT_VERSION);
    if (access (buffer, F_OK) == -1)
      snprintf (buffer, sizeof(buffer), "%s/Muttrc", SYSCONFDIR);
    if (access (buffer, F_OK) == -1)
      snprintf (buffer, sizeof (buffer), "%s/Muttrc-%s", PKGDATADIR, MUTT_VERSION);
    if (access (buffer, F_OK) == -1)
      snprintf (buffer, sizeof (buffer), "%s/Muttrc", PKGDATADIR);
    if (access (buffer, F_OK) != -1)
    {
      if (source_rc (buffer, &err) != 0)
      {
	fputs (err.data, stderr);
	fputc ('\n', stderr);
	need_pause = 1;
      }
    }
  }

  /* Read the user's initialization file.  */
  if (access (Muttrc, F_OK) != -1)
  {
    if (!option (OPTNOCURSES))
      endwin ();
    if (source_rc (Muttrc, &err) != 0)
    {
      fputs (err.data, stderr);
      fputc ('\n', stderr);
      need_pause = 1;
    }
  }
  else if (!default_rc)
  {
    /* file specified by -F does not exist */
    snprintf (buffer, sizeof (buffer), "%s: %s", Muttrc, strerror (errno));
    mutt_endwin (buffer);
    exit (1);
  }

  if (mutt_execute_commands (commands) != 0)
    need_pause = 1;

  if (need_pause && !option (OPTNOCURSES))
  {
    if (mutt_any_key_to_continue (NULL) == -1)
      mutt_exit(1);
  }

  mutt_read_histfile ();

#if 0
  set_option (OPTWEED); /* turn weeding on by default */
#endif
}