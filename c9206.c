int nntp_open_connection(struct NntpAccountData *adata)
{
  struct Connection *conn = adata->conn;
  char buf[256];
  int cap;
  bool posting = false, auth = true;

  if (adata->status == NNTP_OK)
    return 0;
  if (adata->status == NNTP_BYE)
    return -1;
  adata->status = NNTP_NONE;

  if (mutt_socket_open(conn) < 0)
    return -1;

  if (mutt_socket_readln(buf, sizeof(buf), conn) < 0)
    return nntp_connect_error(adata);

  if (mutt_str_startswith(buf, "200", CASE_MATCH))
    posting = true;
  else if (!mutt_str_startswith(buf, "201", CASE_MATCH))
  {
    mutt_socket_close(conn);
    mutt_str_remove_trailing_ws(buf);
    mutt_error("%s", buf);
    return -1;
  }

  /* get initial capabilities */
  cap = nntp_capabilities(adata);
  if (cap < 0)
    return -1;

  /* tell news server to switch to mode reader if it isn't so */
  if (cap > 0)
  {
    if ((mutt_socket_send(conn, "MODE READER\r\n") < 0) ||
        (mutt_socket_readln(buf, sizeof(buf), conn) < 0))
    {
      return nntp_connect_error(adata);
    }

    if (mutt_str_startswith(buf, "200", CASE_MATCH))
      posting = true;
    else if (mutt_str_startswith(buf, "201", CASE_MATCH))
      posting = false;
    /* error if has capabilities, ignore result if no capabilities */
    else if (adata->hasCAPABILITIES)
    {
      mutt_socket_close(conn);
      mutt_error(_("Could not switch to reader mode"));
      return -1;
    }

    /* recheck capabilities after MODE READER */
    if (adata->hasCAPABILITIES)
    {
      cap = nntp_capabilities(adata);
      if (cap < 0)
        return -1;
    }
  }

  mutt_message(_("Connected to %s. %s"), conn->account.host,
               posting ? _("Posting is ok") : _("Posting is NOT ok"));
  mutt_sleep(1);

#ifdef USE_SSL
  /* Attempt STARTTLS if available and desired. */
  if ((adata->use_tls != 1) && (adata->hasSTARTTLS || C_SslForceTls))
  {
    if (adata->use_tls == 0)
    {
      adata->use_tls =
          C_SslForceTls || query_quadoption(C_SslStarttls,
                                            _("Secure connection with TLS?")) == MUTT_YES ?
              2 :
              1;
    }
    if (adata->use_tls == 2)
    {
      if ((mutt_socket_send(conn, "STARTTLS\r\n") < 0) ||
          (mutt_socket_readln(buf, sizeof(buf), conn) < 0))
      {
        return nntp_connect_error(adata);
      }
      if (!mutt_str_startswith(buf, "382", CASE_MATCH))
      {
        adata->use_tls = 0;
        mutt_error("STARTTLS: %s", buf);
      }
      else if (mutt_ssl_starttls(conn))
      {
        adata->use_tls = 0;
        adata->status = NNTP_NONE;
        mutt_socket_close(adata->conn);
        mutt_error(_("Could not negotiate TLS connection"));
        return -1;
      }
      else
      {
        /* recheck capabilities after STARTTLS */
        cap = nntp_capabilities(adata);
        if (cap < 0)
          return -1;
      }
    }
  }
#endif

  /* authentication required? */
  if (conn->account.flags & MUTT_ACCT_USER)
  {
    if (!conn->account.user[0])
      auth = false;
  }
  else
  {
    if ((mutt_socket_send(conn, "STAT\r\n") < 0) ||
        (mutt_socket_readln(buf, sizeof(buf), conn) < 0))
    {
      return nntp_connect_error(adata);
    }
    if (!mutt_str_startswith(buf, "480", CASE_MATCH))
      auth = false;
  }

  /* authenticate */
  if (auth && (nntp_auth(adata) < 0))
    return -1;

  /* get final capabilities after authentication */
  if (adata->hasCAPABILITIES && (auth || (cap > 0)))
  {
    cap = nntp_capabilities(adata);
    if (cap < 0)
      return -1;
    if (cap > 0)
    {
      mutt_socket_close(conn);
      mutt_error(_("Could not switch to reader mode"));
      return -1;
    }
  }

  /* attempt features */
  if (nntp_attempt_features(adata) < 0)
    return -1;

  adata->status = NNTP_OK;
  return 0;
}