main (int argc, char **argv)
{
  int last_argc = -1;
  gpg_error_t err;
  int rc;  parsed_uri_t uri;
  uri_tuple_t r;
  http_t hd;
  int c;
  unsigned int my_http_flags = 0;
  int no_out = 0;
  int tls_dbg = 0;
  int no_crl = 0;
  const char *cafile = NULL;
  http_session_t session = NULL;
  unsigned int timeout = 0;

  gpgrt_init ();
  log_set_prefix (PGM, GPGRT_LOG_WITH_PREFIX | GPGRT_LOG_WITH_PID);
  if (argc)
    { argc--; argv++; }
  while (argc && last_argc != argc )
    {
      last_argc = argc;
      if (!strcmp (*argv, "--"))
        {
          argc--; argv++;
          break;
        }
      else if (!strcmp (*argv, "--help"))
        {
          fputs ("usage: " PGM " URL\n"
                 "Options:\n"
                 "  --verbose         print timings etc.\n"
                 "  --debug           flyswatter\n"
                 "  --tls-debug N     use TLS debug level N\n"
                 "  --cacert FNAME    expect CA certificate in file FNAME\n"
                 "  --timeout MS      timeout for connect in MS\n"
                 "  --no-verify       do not verify the certificate\n"
                 "  --force-tls       use HTTP_FLAG_FORCE_TLS\n"
                 "  --force-tor       use HTTP_FLAG_FORCE_TOR\n"
                 "  --no-out          do not print the content\n"
                 "  --no-crl          do not consuilt a CRL\n",
                 stdout);
          exit (0);
        }
      else if (!strcmp (*argv, "--verbose"))
        {
          verbose++;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--debug"))
        {
          verbose += 2;
          debug++;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--tls-debug"))
        {
          argc--; argv++;
          if (argc)
            {
              tls_dbg = atoi (*argv);
              argc--; argv++;
            }
        }
      else if (!strcmp (*argv, "--cacert"))
        {
          argc--; argv++;
          if (argc)
            {
              cafile = *argv;
              argc--; argv++;
            }
        }
      else if (!strcmp (*argv, "--timeout"))
        {
          argc--; argv++;
          if (argc)
            {
              timeout = strtoul (*argv, NULL, 10);
              argc--; argv++;
            }
        }
      else if (!strcmp (*argv, "--no-verify"))
        {
          no_verify = 1;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--force-tls"))
        {
          my_http_flags |= HTTP_FLAG_FORCE_TLS;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--force-tor"))
        {
          my_http_flags |= HTTP_FLAG_FORCE_TOR;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--no-out"))
        {
          no_out = 1;
          argc--; argv++;
        }
      else if (!strcmp (*argv, "--no-crl"))
        {
          no_crl = 1;
          argc--; argv++;
        }
      else if (!strncmp (*argv, "--", 2))
        {
          fprintf (stderr, PGM ": unknown option '%s'\n", *argv);
          exit (1);
        }
    }
  if (argc != 1)
    {
      fprintf (stderr, PGM ": no or too many URLS given\n");
      exit (1);
    }

  if (!cafile)
    cafile = prepend_srcdir ("tls-ca.pem");

  if (verbose)
    my_http_flags |= HTTP_FLAG_LOG_RESP;

  if (verbose || debug)
    http_set_verbose (verbose, debug);

  /* http.c makes use of the assuan socket wrapper.  */
  assuan_sock_init ();

  if ((my_http_flags & HTTP_FLAG_FORCE_TOR))
    {
      enable_dns_tormode (1);
      if (assuan_sock_set_flag (ASSUAN_INVALID_FD, "tor-mode", 1))
        {
          log_error ("error enabling Tor mode: %s\n", strerror (errno));
          log_info ("(is your Libassuan recent enough?)\n");
        }
    }

#if HTTP_USE_NTBTLS
  log_info ("new session.\n");
  err = http_session_new (&session, NULL,
                          ((no_crl? HTTP_FLAG_NO_CRL : 0)
                           | HTTP_FLAG_TRUST_DEF),
                          my_http_tls_verify_cb, NULL);
  if (err)
    log_error ("http_session_new failed: %s\n", gpg_strerror (err));
  ntbtls_set_debug (tls_dbg, NULL, NULL);

#elif HTTP_USE_GNUTLS

  rc = gnutls_global_init ();
  if (rc)
    log_error ("gnutls_global_init failed: %s\n", gnutls_strerror (rc));

  http_register_tls_callback (verify_callback);
  http_register_tls_ca (cafile);

  err = http_session_new (&session, NULL,
                          ((no_crl? HTTP_FLAG_NO_CRL : 0)
                           | HTTP_FLAG_TRUST_DEF),
                          NULL, NULL);
  if (err)
    log_error ("http_session_new failed: %s\n", gpg_strerror (err));

  /* rc = gnutls_dh_params_init(&dh_params); */
  /* if (rc) */
  /*   log_error ("gnutls_dh_params_init failed: %s\n", gnutls_strerror (rc)); */
  /* read_dh_params ("dh_param.pem"); */

  /* rc = gnutls_certificate_set_x509_trust_file */
  /*   (certcred, "ca.pem", GNUTLS_X509_FMT_PEM); */
  /* if (rc) */
  /*   log_error ("gnutls_certificate_set_x509_trust_file failed: %s\n", */
  /*              gnutls_strerror (rc)); */

  /* gnutls_certificate_set_dh_params (certcred, dh_params); */

  gnutls_global_set_log_function (my_gnutls_log);
  if (tls_dbg)
    gnutls_global_set_log_level (tls_dbg);

#else
  (void)err;
  (void)tls_dbg;
  (void)no_crl;
#endif /*HTTP_USE_GNUTLS*/

  rc = http_parse_uri (&uri, *argv, 1);
  if (rc)
    {
      log_error ("'%s': %s\n", *argv, gpg_strerror (rc));
      return 1;
    }

  printf ("Scheme: %s\n", uri->scheme);
  if (uri->opaque)
    printf ("Value : %s\n", uri->path);
  else
    {
      printf ("Auth  : %s\n", uri->auth? uri->auth:"[none]");
      printf ("Host  : %s\n", uri->host);
      printf ("Port  : %u\n", uri->port);
      printf ("Path  : %s\n", uri->path);
      for (r = uri->params; r; r = r->next)
        {
          printf ("Params: %s", r->name);
          if (!r->no_value)
            {
              printf ("=%s", r->value);
              if (strlen (r->value) != r->valuelen)
                printf (" [real length=%d]", (int) r->valuelen);
            }
          putchar ('\n');
        }
      for (r = uri->query; r; r = r->next)
        {
          printf ("Query : %s", r->name);
          if (!r->no_value)
            {
              printf ("=%s", r->value);
              if (strlen (r->value) != r->valuelen)
                printf (" [real length=%d]", (int) r->valuelen);
            }
          putchar ('\n');
        }
      printf ("Flags :%s%s%s%s\n",
              uri->is_http? " http":"",
              uri->opaque?  " opaque":"",
              uri->v6lit?   " v6lit":"",
              uri->onion?   " onion":"");
      printf ("TLS   : %s\n",
              uri->use_tls? "yes":
              (my_http_flags&HTTP_FLAG_FORCE_TLS)? "forced" : "no");
      printf ("Tor   : %s\n",
              (my_http_flags&HTTP_FLAG_FORCE_TOR)? "yes" : "no");

    }
  fflush (stdout);
  http_release_parsed_uri (uri);
  uri = NULL;

  if (session)
    http_session_set_timeout (session, timeout);

  rc = http_open_document (&hd, *argv, NULL, my_http_flags,
                           NULL, session, NULL, NULL);
  if (rc)
    {
      log_error ("can't get '%s': %s\n", *argv, gpg_strerror (rc));
      return 1;
    }
  log_info ("open_http_document succeeded; status=%u\n",
            http_get_status_code (hd));

  {
    const char **names;
    int i;

    names = http_get_header_names (hd);
    if (!names)
      log_fatal ("http_get_header_names failed: %s\n",
                 gpg_strerror (gpg_error_from_syserror ()));
    for (i = 0; names[i]; i++)
      printf ("HDR: %s: %s\n", names[i], http_get_header (hd, names[i]));
    xfree (names);
  }
  fflush (stdout);

  switch (http_get_status_code (hd))
    {
    case 200:
    case 400:
    case 401:
    case 403:
    case 404:
      {
        unsigned long count = 0;
        while ((c = es_getc (http_get_read_ptr (hd))) != EOF)
          {
            count++;
            if (!no_out)
              putchar (c);
          }
        log_info ("Received bytes: %lu\n", count);
      }
      break;
    case 301:
    case 302:
    case 307:
      log_info ("Redirected to: %s\n", http_get_header (hd, "Location"));
      break;
    }
  http_close (hd, 0);

  http_session_release (session);
#ifdef HTTP_USE_GNUTLS
  gnutls_global_deinit ();
#endif /*HTTP_USE_GNUTLS*/

  return 0;
}