int pop_open_connection(struct PopAccountData *adata)
{
  char buf[1024];

  int rc = pop_connect(adata);
  if (rc < 0)
    return rc;

  rc = pop_capabilities(adata, 0);
  if (rc == -1)
    goto err_conn;
  if (rc == -2)
    return -2;

#ifdef USE_SSL
  /* Attempt STLS if available and desired. */
  if (!adata->conn->ssf && (adata->cmd_stls || C_SslForceTls))
  {
    if (C_SslForceTls)
      adata->use_stls = 2;
    if (adata->use_stls == 0)
    {
      enum QuadOption ans =
          query_quadoption(C_SslStarttls, _("Secure connection with TLS?"));
      if (ans == MUTT_ABORT)
        return -2;
      adata->use_stls = 1;
      if (ans == MUTT_YES)
        adata->use_stls = 2;
    }
    if (adata->use_stls == 2)
    {
      mutt_str_strfcpy(buf, "STLS\r\n", sizeof(buf));
      rc = pop_query(adata, buf, sizeof(buf));
      if (rc == -1)
        goto err_conn;
      if (rc != 0)
      {
        mutt_error("%s", adata->err_msg);
      }
      else if (mutt_ssl_starttls(adata->conn))
      {
        mutt_error(_("Could not negotiate TLS connection"));
        return -2;
      }
      else
      {
        /* recheck capabilities after STLS completes */
        rc = pop_capabilities(adata, 1);
        if (rc == -1)
          goto err_conn;
        if (rc == -2)
          return -2;
      }
    }
  }

  if (C_SslForceTls && !adata->conn->ssf)
  {
    mutt_error(_("Encrypted connection unavailable"));
    return -2;
  }
#endif

  rc = pop_authenticate(adata);
  if (rc == -1)
    goto err_conn;
  if (rc == -3)
    mutt_clear_error();
  if (rc != 0)
    return rc;

  /* recheck capabilities after authentication */
  rc = pop_capabilities(adata, 2);
  if (rc == -1)
    goto err_conn;
  if (rc == -2)
    return -2;

  /* get total size of mailbox */
  mutt_str_strfcpy(buf, "STAT\r\n", sizeof(buf));
  rc = pop_query(adata, buf, sizeof(buf));
  if (rc == -1)
    goto err_conn;
  if (rc == -2)
  {
    mutt_error("%s", adata->err_msg);
    return rc;
  }

  unsigned int n = 0, size = 0;
  sscanf(buf, "+OK %u %u", &n, &size);
  adata->size = size;
  return 0;

err_conn:
  adata->status = POP_DISCONNECTED;
  mutt_error(_("Server closed connection"));
  return -1;
}