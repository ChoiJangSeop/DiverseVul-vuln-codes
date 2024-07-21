send_request (ctrl_t ctrl, const char *request, const char *hostportstr,
              const char *httphost, unsigned int httpflags,
              gpg_error_t (*post_cb)(void *, http_t), void *post_cb_value,
              estream_t *r_fp, unsigned int *r_http_status)
{
  gpg_error_t err;
  http_session_t session = NULL;
  http_t http = NULL;
  int redirects_left = MAX_REDIRECTS;
  estream_t fp = NULL;
  char *request_buffer = NULL;
  parsed_uri_t uri = NULL;
  int is_onion;

  *r_fp = NULL;

  err = http_parse_uri (&uri, request, 0);
  if (err)
    goto leave;
  is_onion = uri->onion;

  err = http_session_new (&session, httphost,
                          ((ctrl->http_no_crl? HTTP_FLAG_NO_CRL : 0)
                           | HTTP_FLAG_TRUST_DEF),
                          gnupg_http_tls_verify_cb, ctrl);
  if (err)
    goto leave;
  http_session_set_log_cb (session, cert_log_cb);
  http_session_set_timeout (session, ctrl->timeout);

 once_more:
  err = http_open (&http,
                   post_cb? HTTP_REQ_POST : HTTP_REQ_GET,
                   request,
                   httphost,
                   /* fixme: AUTH */ NULL,
                   (httpflags
                    |(opt.honor_http_proxy? HTTP_FLAG_TRY_PROXY:0)
                    |(dirmngr_use_tor ()? HTTP_FLAG_FORCE_TOR:0)
                    |(opt.disable_ipv4? HTTP_FLAG_IGNORE_IPv4 : 0)
                    |(opt.disable_ipv6? HTTP_FLAG_IGNORE_IPv6 : 0)),
                   ctrl->http_proxy,
                   session,
                   NULL,
                   /*FIXME curl->srvtag*/NULL);
  if (!err)
    {
      fp = http_get_write_ptr (http);
      /* Avoid caches to get the most recent copy of the key.  We set
         both the Pragma and Cache-Control versions of the header, so
         we're good with both HTTP 1.0 and 1.1.  */
      es_fputs ("Pragma: no-cache\r\n"
                "Cache-Control: no-cache\r\n", fp);
      if (post_cb)
        err = post_cb (post_cb_value, http);
      if (!err)
        {
          http_start_data (http);
          if (es_ferror (fp))
            err = gpg_error_from_syserror ();
        }
    }
  if (err)
    {
      /* Fixme: After a redirection we show the old host name.  */
      log_error (_("error connecting to '%s': %s\n"),
                 hostportstr, gpg_strerror (err));
      goto leave;
    }

  /* Wait for the response.  */
  dirmngr_tick (ctrl);
  err = http_wait_response (http);
  if (err)
    {
      log_error (_("error reading HTTP response for '%s': %s\n"),
                 hostportstr, gpg_strerror (err));
      goto leave;
    }

  if (http_get_tls_info (http, NULL))
    {
      /* Update the httpflags so that a redirect won't fallback to an
         unencrypted connection.  */
      httpflags |= HTTP_FLAG_FORCE_TLS;
    }

  if (r_http_status)
    *r_http_status = http_get_status_code (http);

  switch (http_get_status_code (http))
    {
    case 200:
      err = 0;
      break; /* Success.  */

    case 301:
    case 302:
    case 307:
      {
        const char *s = http_get_header (http, "Location");

        log_info (_("URL '%s' redirected to '%s' (%u)\n"),
                  request, s?s:"[none]", http_get_status_code (http));
        if (s && *s && redirects_left-- )
          {
            if (is_onion)
              {
                /* Make sure that an onion address only redirects to
                 * another onion address.  */
                http_release_parsed_uri (uri);
                uri = NULL;
                err = http_parse_uri (&uri, s, 0);
                if (err)
                  goto leave;

                if (! uri->onion)
                  {
                    err = gpg_error (GPG_ERR_FORBIDDEN);
                    goto leave;
                  }
              }

            xfree (request_buffer);
            request_buffer = xtrystrdup (s);
            if (request_buffer)
              {
                request = request_buffer;
                http_close (http, 0);
                http = NULL;
                goto once_more;
              }
            err = gpg_error_from_syserror ();
          }
        else
          err = gpg_error (GPG_ERR_NO_DATA);
        log_error (_("too many redirections\n"));
      }
      goto leave;

    case 501:
      err = gpg_error (GPG_ERR_NOT_IMPLEMENTED);
      goto leave;

    default:
      log_error (_("error accessing '%s': http status %u\n"),
                 request, http_get_status_code (http));
      err = gpg_error (GPG_ERR_NO_DATA);
      goto leave;
    }

  /* FIXME: We should register a permanent redirection and whether a
     host has ever used TLS so that future calls will always use
     TLS. */

  fp = http_get_read_ptr (http);
  if (!fp)
    {
      err = gpg_error (GPG_ERR_BUG);
      goto leave;
    }

  /* Return the read stream and close the HTTP context.  */
  *r_fp = fp;
  http_close (http, 1);
  http = NULL;

 leave:
  http_close (http, 0);
  http_session_release (session);
  xfree (request_buffer);
  http_release_parsed_uri (uri);
  return err;
}