int imap_open_connection(struct ImapAccountData *adata)
{
  if (mutt_socket_open(adata->conn) < 0)
    return -1;

  adata->state = IMAP_CONNECTED;

  if (imap_cmd_step(adata) != IMAP_RES_OK)
  {
    imap_close_connection(adata);
    return -1;
  }

  if (mutt_str_startswith(adata->buf, "* OK", CASE_IGNORE))
  {
    if (!mutt_str_startswith(adata->buf, "* OK [CAPABILITY", CASE_IGNORE) &&
        check_capabilities(adata))
    {
      goto bail;
    }
#ifdef USE_SSL
    /* Attempt STARTTLS if available and desired. */
    if (!adata->conn->ssf && (C_SslForceTls || (adata->capabilities & IMAP_CAP_STARTTLS)))
    {
      enum QuadOption ans;

      if (C_SslForceTls)
        ans = MUTT_YES;
      else if ((ans = query_quadoption(C_SslStarttls,
                                       _("Secure connection with TLS?"))) == MUTT_ABORT)
      {
        goto err_close_conn;
      }
      if (ans == MUTT_YES)
      {
        enum ImapExecResult rc = imap_exec(adata, "STARTTLS", IMAP_CMD_NO_FLAGS);
        if (rc == IMAP_EXEC_FATAL)
          goto bail;
        if (rc != IMAP_EXEC_ERROR)
        {
          if (mutt_ssl_starttls(adata->conn))
          {
            mutt_error(_("Could not negotiate TLS connection"));
            goto err_close_conn;
          }
          else
          {
            /* RFC2595 demands we recheck CAPABILITY after TLS completes. */
            if (imap_exec(adata, "CAPABILITY", IMAP_CMD_NO_FLAGS))
              goto bail;
          }
        }
      }
    }

    if (C_SslForceTls && !adata->conn->ssf)
    {
      mutt_error(_("Encrypted connection unavailable"));
      goto err_close_conn;
    }
#endif
  }
  else if (mutt_str_startswith(adata->buf, "* PREAUTH", CASE_IGNORE))
  {
#ifdef USE_SSL
    /* An unencrypted PREAUTH response is most likely a MITM attack.
     * Require a confirmation. */
    if (adata->conn->ssf == 0)
    {
      bool proceed = true;
      if (C_SslForceTls)
      {
        proceed = false;
      }
      else if (C_SslStarttls != MUTT_NO)
      {
        proceed = mutt_yesorno(_("Abort unencrypted PREAUTH connection?"),
                               C_SslStarttls) != MUTT_NO;
      }
      if (!proceed)
      {
        mutt_error(_("Encrypted connection unavailable"));
        goto err_close_conn;
      }
    }
#endif

    adata->state = IMAP_AUTHENTICATED;
    if (check_capabilities(adata) != 0)
      goto bail;
    FREE(&adata->capstr);
  }
  else
  {
    imap_error("imap_open_connection()", adata->buf);
    goto bail;
  }

  return 0;

#ifdef USE_SSL
err_close_conn:
  imap_close_connection(adata);
#endif
bail:
  FREE(&adata->capstr);
  return -1;
}