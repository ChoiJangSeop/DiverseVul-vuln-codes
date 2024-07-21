int imap_exec(struct ImapAccountData *adata, const char *cmdstr, ImapCmdFlags flags)
{
  int rc;

  rc = cmd_start(adata, cmdstr, flags);
  if (rc < 0)
  {
    cmd_handle_fatal(adata);
    return IMAP_EXEC_FATAL;
  }

  if (flags & IMAP_CMD_QUEUE)
    return IMAP_EXEC_SUCCESS;

  if ((flags & IMAP_CMD_POLL) && (C_ImapPollTimeout > 0) &&
      ((mutt_socket_poll(adata->conn, C_ImapPollTimeout)) == 0))
  {
    mutt_error(_("Connection to %s timed out"), adata->conn->account.host);
    cmd_handle_fatal(adata);
    return IMAP_EXEC_FATAL;
  }

  /* Allow interruptions, particularly useful if there are network problems. */
  mutt_sig_allow_interrupt(true);
  do
  {
    rc = imap_cmd_step(adata);
  } while (rc == IMAP_RES_CONTINUE);
  mutt_sig_allow_interrupt(false);

  if (rc == IMAP_RES_NO)
    return IMAP_EXEC_ERROR;
  if (rc != IMAP_RES_OK)
  {
    if (adata->status != IMAP_FATAL)
      return IMAP_EXEC_ERROR;

    mutt_debug(LL_DEBUG1, "command failed: %s\n", adata->buf);
    return IMAP_EXEC_FATAL;
  }

  return IMAP_EXEC_SUCCESS;
}