static int smtp_open(struct Connection *conn, bool esmtp)
{
  int rc;

  if (mutt_socket_open(conn))
    return -1;

  /* get greeting string */
  rc = smtp_get_resp(conn);
  if (rc != 0)
    return rc;

  rc = smtp_helo(conn, esmtp);
  if (rc != 0)
    return rc;

#ifdef USE_SSL
  enum QuadOption ans = MUTT_NO;
  if (conn->ssf)
    ans = MUTT_NO;
  else if (C_SslForceTls)
    ans = MUTT_YES;
  else if ((Capabilities & SMTP_CAP_STARTTLS) &&
           ((ans = query_quadoption(C_SslStarttls,
                                    _("Secure connection with TLS?"))) == MUTT_ABORT))
  {
    return -1;
  }

  if (ans == MUTT_YES)
  {
    if (mutt_socket_send(conn, "STARTTLS\r\n") < 0)
      return SMTP_ERR_WRITE;
    rc = smtp_get_resp(conn);
    if (rc != 0)
      return rc;

    if (mutt_ssl_starttls(conn))
    {
      mutt_error(_("Could not negotiate TLS connection"));
      return -1;
    }

    /* re-EHLO to get authentication mechanisms */
    rc = smtp_helo(conn, esmtp);
    if (rc != 0)
      return rc;
  }
#endif

  if (conn->account.flags & MUTT_ACCT_USER)
  {
    if (!(Capabilities & SMTP_CAP_AUTH))
    {
      mutt_error(_("SMTP server does not support authentication"));
      return -1;
    }

    return smtp_auth(conn);
  }

  return 0;
}