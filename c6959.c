gethttp (const struct url *u, struct url *original_url, struct http_stat *hs,
         int *dt, struct url *proxy, struct iri *iri, int count)
{
  struct request *req = NULL;

  char *type = NULL;
  char *user, *passwd;
  char *proxyauth;
  int statcode;
  int write_error;
  wgint contlen, contrange;
  const struct url *conn;
  FILE *fp;
  int err;
  uerr_t retval;
#ifdef HAVE_HSTS
#ifdef TESTING
  /* we don't link against main.o when we're testing */
  hsts_store_t hsts_store = NULL;
#else
  extern hsts_store_t hsts_store;
#endif
  const char *hsts_params;
  time_t max_age;
  bool include_subdomains;
#endif

  int sock = -1;

  /* Set to 1 when the authorization has already been sent and should
     not be tried again. */
  bool auth_finished = false;

  /* Set to 1 when just globally-set Basic authorization has been sent;
   * should prevent further Basic negotiations, but not other
   * mechanisms. */
  bool basic_auth_finished = false;

  /* Whether NTLM authentication is used for this request. */
  bool ntlm_seen = false;

  /* Whether our connection to the remote host is through SSL.  */
  bool using_ssl = false;

  /* Whether a HEAD request will be issued (as opposed to GET or
     POST). */
  bool head_only = !!(*dt & HEAD_ONLY);

  /* Whether conditional get request will be issued.  */
  bool cond_get = !!(*dt & IF_MODIFIED_SINCE);

#ifdef HAVE_METALINK
  /* Are we looking for metalink info in HTTP headers?  */
  bool metalink = !!(*dt & METALINK_METADATA);
#endif

  char *head = NULL;
  struct response *resp = NULL;
  char hdrval[512];
  char *message = NULL;

  /* Declare WARC variables. */
  bool warc_enabled = (opt.warc_filename != NULL);
  FILE *warc_tmp = NULL;
  char warc_timestamp_str [21];
  char warc_request_uuid [48];
  ip_address *warc_ip = NULL;
  off_t warc_payload_offset = -1;

  /* Whether this connection will be kept alive after the HTTP request
     is done. */
  bool keep_alive;

  /* Is the server using the chunked transfer encoding?  */
  bool chunked_transfer_encoding = false;

  /* Whether keep-alive should be inhibited.  */
  bool inhibit_keep_alive =
    !opt.http_keep_alive || opt.ignore_length;

  /* Headers sent when using POST. */
  wgint body_data_size = 0;

#ifdef HAVE_SSL
  if (u->scheme == SCHEME_HTTPS)
    {
      /* Initialize the SSL context.  After this has once been done,
         it becomes a no-op.  */
      if (!ssl_init ())
        {
          scheme_disable (SCHEME_HTTPS);
          logprintf (LOG_NOTQUIET,
                     _("Disabling SSL due to encountered errors.\n"));
          retval = SSLINITFAILED;
          goto cleanup;
        }
    }
#endif /* HAVE_SSL */

  /* Initialize certain elements of struct http_stat.  */
  hs->len = 0;
  hs->contlen = -1;
  hs->res = -1;
  hs->rderrmsg = NULL;
  hs->newloc = NULL;
  xfree (hs->remote_time);
  hs->error = NULL;
  hs->message = NULL;
  hs->local_encoding = ENC_NONE;
  hs->remote_encoding = ENC_NONE;

  conn = u;

  {
    uerr_t ret;
    req = initialize_request (u, hs, dt, proxy, inhibit_keep_alive,
                              &basic_auth_finished, &body_data_size,
                              &user, &passwd, &ret);
    if (req == NULL)
      {
        retval = ret;
        goto cleanup;
      }
  }
 retry_with_auth:
  /* We need to come back here when the initial attempt to retrieve
     without authorization header fails.  (Expected to happen at least
     for the Digest authorization scheme.)  */

  if (opt.cookies)
    request_set_header (req, "Cookie",
                        cookie_header (wget_cookie_jar,
                                       u->host, u->port, u->path,
#ifdef HAVE_SSL
                                       u->scheme == SCHEME_HTTPS
#else
                                       0
#endif
                                       ),
                        rel_value);

  /* Add the user headers. */
  if (opt.user_headers)
    {
      int i;
      for (i = 0; opt.user_headers[i]; i++)
        request_set_user_header (req, opt.user_headers[i]);
    }

  proxyauth = NULL;
  if (proxy)
    {
      conn = proxy;
      initialize_proxy_configuration (u, req, proxy, &proxyauth);
    }
  keep_alive = true;

  /* Establish the connection.  */
  if (inhibit_keep_alive)
    keep_alive = false;

  {
    uerr_t conn_err = establish_connection (u, &conn, hs, proxy, &proxyauth, &req,
                                            &using_ssl, inhibit_keep_alive, &sock);
    if (conn_err != RETROK)
      {
        retval = conn_err;
        goto cleanup;
      }
  }

  /* Open the temporary file where we will write the request. */
  if (warc_enabled)
    {
      warc_tmp = warc_tempfile ();
      if (warc_tmp == NULL)
        {
          CLOSE_INVALIDATE (sock);
          retval = WARC_TMP_FOPENERR;
          goto cleanup;
        }

      if (! proxy)
        {
          warc_ip = (ip_address *) alloca (sizeof (ip_address));
          socket_ip_address (sock, warc_ip, ENDPOINT_PEER);
        }
    }

  /* Send the request to server.  */
  write_error = request_send (req, sock, warc_tmp);

  if (write_error >= 0)
    {
      if (opt.body_data)
        {
          DEBUGP (("[BODY data: %s]\n", opt.body_data));
          write_error = fd_write (sock, opt.body_data, body_data_size, -1);
          if (write_error >= 0 && warc_tmp != NULL)
            {
              int warc_tmp_written;

              /* Remember end of headers / start of payload. */
              warc_payload_offset = ftello (warc_tmp);

              /* Write a copy of the data to the WARC record. */
              warc_tmp_written = fwrite (opt.body_data, 1, body_data_size, warc_tmp);
              if (warc_tmp_written != body_data_size)
                write_error = -2;
            }
         }
      else if (opt.body_file && body_data_size != 0)
        {
          if (warc_tmp != NULL)
            /* Remember end of headers / start of payload */
            warc_payload_offset = ftello (warc_tmp);

          write_error = body_file_send (sock, opt.body_file, body_data_size, warc_tmp);
        }
    }

  if (write_error < 0)
    {
      CLOSE_INVALIDATE (sock);

      if (warc_tmp != NULL)
        fclose (warc_tmp);

      if (write_error == -2)
        retval = WARC_TMP_FWRITEERR;
      else
        retval = WRITEFAILED;
      goto cleanup;
    }
  logprintf (LOG_VERBOSE, _("%s request sent, awaiting response... "),
             proxy ? "Proxy" : "HTTP");
  contlen = -1;
  contrange = 0;
  *dt &= ~RETROKF;


  if (warc_enabled)
    {
      bool warc_result;

      /* Generate a timestamp and uuid for this request. */
      warc_timestamp (warc_timestamp_str, sizeof (warc_timestamp_str));
      warc_uuid_str (warc_request_uuid);

      /* Create a request record and store it in the WARC file. */
      warc_result = warc_write_request_record (u->url, warc_timestamp_str,
                                               warc_request_uuid, warc_ip,
                                               warc_tmp, warc_payload_offset);
      if (! warc_result)
        {
          CLOSE_INVALIDATE (sock);
          retval = WARC_ERR;
          goto cleanup;
        }

      /* warc_write_request_record has also closed warc_tmp. */
    }

  /* Repeat while we receive a 10x response code.  */
  {
    bool _repeat;

    do
      {
        head = read_http_response_head (sock);
        if (!head)
          {
            if (errno == 0)
              {
                logputs (LOG_NOTQUIET, _("No data received.\n"));
                CLOSE_INVALIDATE (sock);
                retval = HEOF;
              }
            else
              {
                logprintf (LOG_NOTQUIET, _("Read error (%s) in headers.\n"),
                           fd_errstr (sock));
                CLOSE_INVALIDATE (sock);
                retval = HERR;
              }
            goto cleanup;
          }
        DEBUGP (("\n---response begin---\n%s---response end---\n", head));

        resp = resp_new (head);

        /* Check for status line.  */
        xfree (message);
        statcode = resp_status (resp, &message);
        if (statcode < 0)
          {
            char *tms = datetime_str (time (NULL));
            logprintf (LOG_VERBOSE, "%d\n", statcode);
            logprintf (LOG_NOTQUIET, _("%s ERROR %d: %s.\n"), tms, statcode,
                       quotearg_style (escape_quoting_style,
                                       _("Malformed status line")));
            CLOSE_INVALIDATE (sock);
            retval = HERR;
            goto cleanup;
          }

        if (H_10X (statcode))
          {
            xfree (head);
            resp_free (&resp);
            _repeat = true;
            DEBUGP (("Ignoring response\n"));
          }
        else
          {
            _repeat = false;
          }
      }
    while (_repeat);
  }

  xfree (hs->message);
  hs->message = xstrdup (message);
  if (!opt.server_response)
    logprintf (LOG_VERBOSE, "%2d %s\n", statcode,
               message ? quotearg_style (escape_quoting_style, message) : "");
  else
    {
      logprintf (LOG_VERBOSE, "\n");
      print_server_response (resp, "  ");
    }

  if (!opt.ignore_length
      && resp_header_copy (resp, "Content-Length", hdrval, sizeof (hdrval)))
    {
      wgint parsed;
      errno = 0;
      parsed = str_to_wgint (hdrval, NULL, 10);
      if (parsed == WGINT_MAX && errno == ERANGE)
        {
          /* Out of range.
             #### If Content-Length is out of range, it most likely
             means that the file is larger than 2G and that we're
             compiled without LFS.  In that case we should probably
             refuse to even attempt to download the file.  */
          contlen = -1;
        }
      else if (parsed < 0)
        {
          /* Negative Content-Length; nonsensical, so we can't
             assume any information about the content to receive. */
          contlen = -1;
        }
      else
        contlen = parsed;
    }

  /* Check for keep-alive related responses. */
  if (!inhibit_keep_alive)
    {
      if (resp_header_copy (resp, "Connection", hdrval, sizeof (hdrval)))
        {
          if (0 == c_strcasecmp (hdrval, "Close"))
            keep_alive = false;
        }
    }

  chunked_transfer_encoding = false;
  if (resp_header_copy (resp, "Transfer-Encoding", hdrval, sizeof (hdrval))
      && 0 == c_strcasecmp (hdrval, "chunked"))
    chunked_transfer_encoding = true;

  /* Handle (possibly multiple instances of) the Set-Cookie header. */
  if (opt.cookies)
    {
      int scpos;
      const char *scbeg, *scend;
      /* The jar should have been created by now. */
      assert (wget_cookie_jar != NULL);
      for (scpos = 0;
           (scpos = resp_header_locate (resp, "Set-Cookie", scpos,
                                        &scbeg, &scend)) != -1;
           ++scpos)
        {
          char *set_cookie; BOUNDED_TO_ALLOCA (scbeg, scend, set_cookie);
          cookie_handle_set_cookie (wget_cookie_jar, u->host, u->port,
                                    u->path, set_cookie);
        }
    }

  if (keep_alive)
    /* The server has promised that it will not close the connection
       when we're done.  This means that we can register it.  */
    register_persistent (conn->host, conn->port, sock, using_ssl);

#ifdef HAVE_METALINK
  /* We need to check for the Metalink data in the very first response
     we get from the server (before redirections, authorization, etc.).  */
  if (metalink)
    {
      hs->metalink = metalink_from_http (resp, hs, u);
      /* Bugfix: hs->local_file is NULL (opt.content_disposition).  */
      if (!hs->local_file && hs->metalink && hs->metalink->origin)
        hs->local_file = xstrdup (hs->metalink->origin);
      xfree (hs->message);
      retval = RETR_WITH_METALINK;
      CLOSE_FINISH (sock);
      goto cleanup;
    }
#endif

  if (statcode == HTTP_STATUS_UNAUTHORIZED)
    {
      /* Authorization is required.  */
      uerr_t auth_err = RETROK;
      bool retry;
      /* Normally we are not interested in the response body.
         But if we are writing a WARC file we are: we like to keep everything.  */
      if (warc_enabled)
        {
          int _err;
          type = resp_header_strdup (resp, "Content-Type");
          _err = read_response_body (hs, sock, NULL, contlen, 0,
                                    chunked_transfer_encoding,
                                    u->url, warc_timestamp_str,
                                    warc_request_uuid, warc_ip, type,
                                    statcode, head);
          xfree (type);

          if (_err != RETRFINISHED || hs->res < 0)
            {
              CLOSE_INVALIDATE (sock);
              retval = _err;
              goto cleanup;
            }
          else
            CLOSE_FINISH (sock);
        }
      else
        {
          /* Since WARC is disabled, we are not interested in the response body.  */
          if (keep_alive && !head_only
              && skip_short_body (sock, contlen, chunked_transfer_encoding))
            CLOSE_FINISH (sock);
          else
            CLOSE_INVALIDATE (sock);
        }

      pconn.authorized = false;

      {
        auth_err = check_auth (u, user, passwd, resp, req,
                               &ntlm_seen, &retry,
                               &basic_auth_finished,
                               &auth_finished);
        if (auth_err == RETROK && retry)
          {
            xfree (hs->message);
            resp_free (&resp);
            xfree (message);
            xfree (head);
            goto retry_with_auth;
          }
      }
      if (auth_err == RETROK)
        retval = AUTHFAILED;
      else
        retval = auth_err;
      goto cleanup;
    }
  else /* statcode != HTTP_STATUS_UNAUTHORIZED */
    {
      /* Kludge: if NTLM is used, mark the TCP connection as authorized. */
      if (ntlm_seen)
        pconn.authorized = true;
    }

  {
    uerr_t ret = check_file_output (u, hs, resp, hdrval, sizeof hdrval);
    if (ret != RETROK)
      {
        retval = ret;
        goto cleanup;
      }
  }

  hs->statcode = statcode;
  if (statcode == -1)
    hs->error = xstrdup (_("Malformed status line"));
  else if (!*message)
    hs->error = xstrdup (_("(no description)"));
  else
    hs->error = xstrdup (message);

#ifdef HAVE_HSTS
  if (opt.hsts && hsts_store)
    {
      hsts_params = resp_header_strdup (resp, "Strict-Transport-Security");
      if (parse_strict_transport_security (hsts_params, &max_age, &include_subdomains))
        {
          /* process strict transport security */
          if (hsts_store_entry (hsts_store, u->scheme, u->host, u->port, max_age, include_subdomains))
            DEBUGP(("Added new HSTS host: %s:%u (max-age: %lu, includeSubdomains: %s)\n",
                   u->host,
                   (unsigned) u->port,
                   (unsigned long) max_age,
                   (include_subdomains ? "true" : "false")));
          else
            DEBUGP(("Updated HSTS host: %s:%u (max-age: %lu, includeSubdomains: %s)\n",
                   u->host,
                   (unsigned) u->port,
                   (unsigned long) max_age,
                   (include_subdomains ? "true" : "false")));
        }
    }
#endif

  type = resp_header_strdup (resp, "Content-Type");
  if (type)
    {
      char *tmp = strchr (type, ';');
      if (tmp)
        {
#ifdef ENABLE_IRI
          /* sXXXav: only needed if IRI support is enabled */
          char *tmp2 = tmp + 1;
#endif

          while (tmp > type && c_isspace (tmp[-1]))
            --tmp;
          *tmp = '\0';

#ifdef ENABLE_IRI
          /* Try to get remote encoding if needed */
          if (opt.enable_iri && !opt.encoding_remote)
            {
              tmp = parse_charset (tmp2);
              if (tmp)
                set_content_encoding (iri, tmp);
              xfree (tmp);
            }
#endif
        }
    }
  hs->newloc = resp_header_strdup (resp, "Location");
  hs->remote_time = resp_header_strdup (resp, "Last-Modified");
  if (!hs->remote_time) // now look for the Wayback Machine's timestamp
    hs->remote_time = resp_header_strdup (resp, "X-Archive-Orig-last-modified");

  if (resp_header_copy (resp, "Content-Range", hdrval, sizeof (hdrval)))
    {
      wgint first_byte_pos, last_byte_pos, entity_length;
      if (parse_content_range (hdrval, &first_byte_pos, &last_byte_pos,
                               &entity_length))
        {
          contrange = first_byte_pos;
          contlen = last_byte_pos - first_byte_pos + 1;
        }
    }

  if (resp_header_copy (resp, "Content-Encoding", hdrval, sizeof (hdrval)))
    {
      hs->local_encoding = ENC_INVALID;

      switch (hdrval[0])
        {
        case 'b': case 'B':
          if (0 == c_strcasecmp(hdrval, "br"))
            hs->local_encoding = ENC_BROTLI;
          break;
        case 'c': case 'C':
          if (0 == c_strcasecmp(hdrval, "compress"))
            hs->local_encoding = ENC_COMPRESS;
          break;
        case 'd': case 'D':
          if (0 == c_strcasecmp(hdrval, "deflate"))
            hs->local_encoding = ENC_DEFLATE;
          break;
        case 'g': case 'G':
          if (0 == c_strcasecmp(hdrval, "gzip"))
            hs->local_encoding = ENC_GZIP;
          break;
        case 'i': case 'I':
          if (0 == c_strcasecmp(hdrval, "identity"))
            hs->local_encoding = ENC_NONE;
          break;
        case 'x': case 'X':
          if (0 == c_strcasecmp(hdrval, "x-compress"))
            hs->local_encoding = ENC_COMPRESS;
          else if (0 == c_strcasecmp(hdrval, "x-gzip"))
            hs->local_encoding = ENC_GZIP;
          break;
        case '\0':
          hs->local_encoding = ENC_NONE;
        }

      if (hs->local_encoding == ENC_INVALID)
        {
          DEBUGP (("Unrecognized Content-Encoding: %s\n", hdrval));
          hs->local_encoding = ENC_NONE;
        }
#ifdef HAVE_LIBZ
      else if (hs->local_encoding == ENC_GZIP
               && opt.compression != compression_none)
        {
          const char *p;

          /* Make sure the Content-Type is not gzip before decompressing */
          if (type)
            {
              p = strchr (type, '/');
              if (p == NULL)
                {
                  hs->remote_encoding = ENC_GZIP;
                  hs->local_encoding = ENC_NONE;
                }
              else
                {
                  p++;
                  if (c_tolower(p[0]) == 'x' && p[1] == '-')
                    p += 2;
                  if (0 != c_strcasecmp (p, "gzip"))
                    {
                      hs->remote_encoding = ENC_GZIP;
                      hs->local_encoding = ENC_NONE;
                    }
                }
            }
          else
            {
               hs->remote_encoding = ENC_GZIP;
               hs->local_encoding = ENC_NONE;
            }

          /* don't uncompress if a file ends with '.gz' or '.tgz' */
          if (hs->remote_encoding == ENC_GZIP
              && (p = strrchr(u->file, '.'))
              && (c_strcasecmp(p, ".gz") == 0 || c_strcasecmp(p, ".tgz") == 0))
            {
               DEBUGP (("Enabling broken server workaround. Will not decompress this GZip file.\n"));
               hs->remote_encoding = ENC_NONE;
            }
        }
#endif
    }

  /* 20x responses are counted among successful by default.  */
  if (H_20X (statcode))
    *dt |= RETROKF;

  if (statcode == HTTP_STATUS_NO_CONTENT)
    {
      /* 204 response has no body (RFC 2616, 4.3) */

      /* In case the caller cares to look...  */
      hs->len = 0;
      hs->res = 0;
      hs->restval = 0;

      CLOSE_FINISH (sock);

      retval = RETRFINISHED;
      goto cleanup;
    }

  /* Return if redirected.  */
  if (H_REDIRECTED (statcode) || statcode == HTTP_STATUS_MULTIPLE_CHOICES)
    {
      /* RFC2068 says that in case of the 300 (multiple choices)
         response, the server can output a preferred URL through
         `Location' header; otherwise, the request should be treated
         like GET.  So, if the location is set, it will be a
         redirection; otherwise, just proceed normally.  */
      if (statcode == HTTP_STATUS_MULTIPLE_CHOICES && !hs->newloc)
        *dt |= RETROKF;
      else
        {
          logprintf (LOG_VERBOSE,
                     _("Location: %s%s\n"),
                     hs->newloc ? escnonprint_uri (hs->newloc) : _("unspecified"),
                     hs->newloc ? _(" [following]") : "");

          /* In case the caller cares to look...  */
          hs->len = 0;
          hs->res = 0;
          hs->restval = 0;

          /* Normally we are not interested in the response body of a redirect.
             But if we are writing a WARC file we are: we like to keep everything.  */
          if (warc_enabled)
            {
              int _err = read_response_body (hs, sock, NULL, contlen, 0,
                                            chunked_transfer_encoding,
                                            u->url, warc_timestamp_str,
                                            warc_request_uuid, warc_ip, type,
                                            statcode, head);

              if (_err != RETRFINISHED || hs->res < 0)
                {
                  CLOSE_INVALIDATE (sock);
                  retval = _err;
                  goto cleanup;
                }
              else
                CLOSE_FINISH (sock);
            }
          else
            {
              /* Since WARC is disabled, we are not interested in the response body.  */
              if (keep_alive && !head_only
                  && skip_short_body (sock, contlen, chunked_transfer_encoding))
                CLOSE_FINISH (sock);
              else
                CLOSE_INVALIDATE (sock);
            }

          /* From RFC2616: The status codes 303 and 307 have
             been added for servers that wish to make unambiguously
             clear which kind of reaction is expected of the client.

             A 307 should be redirected using the same method,
             in other words, a POST should be preserved and not
             converted to a GET in that case.

             With strict adherence to RFC2616, POST requests are not
             converted to a GET request on 301 Permanent Redirect
             or 302 Temporary Redirect.

             A switch may be provided later based on the HTTPbis draft
             that allows clients to convert POST requests to GET
             requests on 301 and 302 response codes. */
          switch (statcode)
            {
            case HTTP_STATUS_TEMPORARY_REDIRECT:
            case HTTP_STATUS_PERMANENT_REDIRECT:
              retval = NEWLOCATION_KEEP_POST;
              goto cleanup;
            case HTTP_STATUS_MOVED_PERMANENTLY:
              if (opt.method && c_strcasecmp (opt.method, "post") != 0)
                {
                  retval = NEWLOCATION_KEEP_POST;
                  goto cleanup;
                }
              break;
            case HTTP_STATUS_MOVED_TEMPORARILY:
              if (opt.method && c_strcasecmp (opt.method, "post") != 0)
                {
                  retval = NEWLOCATION_KEEP_POST;
                  goto cleanup;
                }
              break;
            }
          retval = NEWLOCATION;
          goto cleanup;
        }
    }

  if (cond_get)
    {
      if (statcode == HTTP_STATUS_NOT_MODIFIED)
        {
          logprintf (LOG_VERBOSE,
                     _ ("File %s not modified on server. Omitting download.\n\n"),
                     quote (hs->local_file));
          *dt |= RETROKF;
          CLOSE_FINISH (sock);
          retval = RETRUNNEEDED;
          goto cleanup;
        }
    }

  set_content_type (dt, type);

  if (opt.adjust_extension)
    {
      const char *encoding_ext = NULL;
      switch (hs->local_encoding)
        {
        case ENC_INVALID:
        case ENC_NONE:
          break;
        case ENC_BROTLI:
          encoding_ext = ".br";
          break;
        case ENC_COMPRESS:
          encoding_ext = ".Z";
          break;
        case ENC_DEFLATE:
          encoding_ext = ".zlib";
          break;
        case ENC_GZIP:
          encoding_ext = ".gz";
          break;
        default:
          DEBUGP (("No extension found for encoding %d\n",
                   hs->local_encoding));
      }
      if (encoding_ext != NULL)
        {
          char *file_ext = strrchr (hs->local_file, '.');
          /* strip Content-Encoding extension (it will be re-added later) */
          if (file_ext != NULL && 0 == strcasecmp (file_ext, encoding_ext))
            *file_ext = '\0';
        }
      if (*dt & TEXTHTML)
        /* -E / --adjust-extension / adjust_extension = on was specified,
           and this is a text/html file.  If some case-insensitive
           variation on ".htm[l]" isn't already the file's suffix,
           tack on ".html". */
        {
          ensure_extension (hs, ".html", dt);
        }
      else if (*dt & TEXTCSS)
        {
          ensure_extension (hs, ".css", dt);
        }
      if (encoding_ext != NULL)
        {
          ensure_extension (hs, encoding_ext, dt);
        }
    }

  if (cond_get)
    {
      /* Handle the case when server ignores If-Modified-Since header.  */
      if (statcode == HTTP_STATUS_OK && hs->remote_time)
        {
          time_t tmr = http_atotm (hs->remote_time);

          /* Check if the local file is up-to-date based on Last-Modified header
             and content length.  */
          if (tmr != (time_t) - 1 && tmr <= hs->orig_file_tstamp
              && (contlen == -1 || contlen == hs->orig_file_size))
            {
              logprintf (LOG_VERBOSE,
                         _("Server ignored If-Modified-Since header for file %s.\n"
                           "You might want to add --no-if-modified-since option."
                           "\n\n"),
                         quote (hs->local_file));
              *dt |= RETROKF;
              CLOSE_INVALIDATE (sock);
              retval = RETRUNNEEDED;
              goto cleanup;
            }
        }
    }

  if (statcode == HTTP_STATUS_RANGE_NOT_SATISFIABLE
      || (!opt.timestamping && hs->restval > 0 && statcode == HTTP_STATUS_OK
          && contrange == 0 && contlen >= 0 && hs->restval >= contlen))
    {
      /* If `-c' is in use and the file has been fully downloaded (or
         the remote file has shrunk), Wget effectively requests bytes
         after the end of file and the server response with 416
         (or 200 with a <= Content-Length.  */
      logputs (LOG_VERBOSE, _("\
\n    The file is already fully retrieved; nothing to do.\n\n"));
      /* In case the caller inspects. */
      hs->len = contlen;
      hs->res = 0;
      /* Mark as successfully retrieved. */
      *dt |= RETROKF;

      /* Try to maintain the keep-alive connection. It is often cheaper to
       * consume some bytes which have already been sent than to negotiate
       * a new connection. However, if the body is too large, or we don't
       * care about keep-alive, then simply terminate the connection */
      if (keep_alive &&
          skip_short_body (sock, contlen, chunked_transfer_encoding))
        CLOSE_FINISH (sock);
      else
        CLOSE_INVALIDATE (sock);
      retval = RETRUNNEEDED;
      goto cleanup;
    }
  if ((contrange != 0 && contrange != hs->restval)
      || (H_PARTIAL (statcode) && !contrange && hs->restval))
    {
      /* The Range request was somehow misunderstood by the server.
         Bail out.  */
      CLOSE_INVALIDATE (sock);
      retval = RANGEERR;
      goto cleanup;
    }
  if (contlen == -1)
    hs->contlen = -1;
  /* If the response is gzipped, the uncompressed size is unknown. */
  else if (hs->remote_encoding == ENC_GZIP)
    hs->contlen = -1;
  else
    hs->contlen = contlen + contrange;

  if (opt.verbose)
    {
      if (*dt & RETROKF)
        {
          /* No need to print this output if the body won't be
             downloaded at all, or if the original server response is
             printed.  */
          logputs (LOG_VERBOSE, _("Length: "));
          if (contlen != -1)
            {
              logputs (LOG_VERBOSE, number_to_static_string (contlen + contrange));
              if (contlen + contrange >= 1024)
                logprintf (LOG_VERBOSE, " (%s)",
                           human_readable (contlen + contrange, 10, 1));
              if (contrange)
                {
                  if (contlen >= 1024)
                    logprintf (LOG_VERBOSE, _(", %s (%s) remaining"),
                               number_to_static_string (contlen),
                               human_readable (contlen, 10, 1));
                  else
                    logprintf (LOG_VERBOSE, _(", %s remaining"),
                               number_to_static_string (contlen));
                }
            }
          else
            logputs (LOG_VERBOSE,
                     opt.ignore_length ? _("ignored") : _("unspecified"));
          if (type)
            logprintf (LOG_VERBOSE, " [%s]\n", quotearg_style (escape_quoting_style, type));
          else
            logputs (LOG_VERBOSE, "\n");
        }
    }

  /* Return if we have no intention of further downloading.  */
  if ((!(*dt & RETROKF) && !opt.content_on_error) || head_only || (opt.spider && !opt.recursive))
    {
      /* In case the caller cares to look...  */
      hs->len = 0;
      hs->res = 0;
      hs->restval = 0;

      /* Normally we are not interested in the response body of a error responses.
         But if we are writing a WARC file we are: we like to keep everything.  */
      if (warc_enabled)
        {
          int _err = read_response_body (hs, sock, NULL, contlen, 0,
                                        chunked_transfer_encoding,
                                        u->url, warc_timestamp_str,
                                        warc_request_uuid, warc_ip, type,
                                        statcode, head);

          if (_err != RETRFINISHED || hs->res < 0)
            {
              CLOSE_INVALIDATE (sock);
              retval = _err;
              goto cleanup;
            }

          CLOSE_FINISH (sock);
        }
      else
        {
          /* Since WARC is disabled, we are not interested in the response body.  */
          if (head_only)
            /* Pre-1.10 Wget used CLOSE_INVALIDATE here.  Now we trust the
               servers not to send body in response to a HEAD request, and
               those that do will likely be caught by test_socket_open.
               If not, they can be worked around using
               `--no-http-keep-alive'.  */
            CLOSE_FINISH (sock);
          else if (opt.spider && !opt.recursive)
            /* we just want to see if the page exists - no downloading required */
            CLOSE_INVALIDATE (sock);
          else if (keep_alive
                   && skip_short_body (sock, contlen, chunked_transfer_encoding))
            /* Successfully skipped the body; also keep using the socket. */
            CLOSE_FINISH (sock);
          else
            CLOSE_INVALIDATE (sock);
        }

      if (statcode == HTTP_STATUS_GATEWAY_TIMEOUT)
        retval = GATEWAYTIMEOUT;
      else
        retval = RETRFINISHED;

      goto cleanup;
    }

  err = open_output_stream (hs, count, &fp);
  if (err != RETROK)
    {
      CLOSE_INVALIDATE (sock);
      retval = err;
      goto cleanup;
    }

#ifdef ENABLE_XATTR
  if (opt.enable_xattr)
    {
      if (original_url != u)
        set_file_metadata (u->url, original_url->url, fp);
      else
        set_file_metadata (u->url, NULL, fp);
    }
#endif

  err = read_response_body (hs, sock, fp, contlen, contrange,
                            chunked_transfer_encoding,
                            u->url, warc_timestamp_str,
                            warc_request_uuid, warc_ip, type,
                            statcode, head);

  if (hs->res >= 0)
    CLOSE_FINISH (sock);
  else
    CLOSE_INVALIDATE (sock);

  if (!output_stream)
    fclose (fp);

  retval = err;

  cleanup:
  xfree (head);
  xfree (type);
  xfree (message);
  resp_free (&resp);
  request_free (&req);

  return retval;
}