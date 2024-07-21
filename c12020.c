nbd_internal_command_common (struct nbd_handle *h,
                             uint16_t flags, uint16_t type,
                             uint64_t offset, uint64_t count, int count_err,
                             void *data, struct command_cb *cb)
{
  struct command *cmd;

  if (h->disconnect_request) {
      set_error (EINVAL, "cannot request more commands after NBD_CMD_DISC");
      goto err;
  }
  if (h->in_flight == INT_MAX) {
      set_error (ENOMEM, "too many commands already in flight");
      goto err;
  }

  if (count_err) {
    if ((h->strict & LIBNBD_STRICT_ZERO_SIZE) && count == 0) {
      set_error (EINVAL, "count cannot be 0");
      goto err;
    }

    if ((h->strict & LIBNBD_STRICT_BOUNDS) &&
        (offset > h->exportsize || count > h->exportsize - offset)) {
      set_error (count_err, "request out of bounds");
      goto err;
    }

    if (h->block_minimum && (h->strict & LIBNBD_STRICT_ALIGN) &&
        (offset | count) & (h->block_minimum - 1)) {
      set_error (EINVAL, "request is unaligned");
      goto err;
    }
  }

  switch (type) {
    /* Commands which send or receive data are limited to MAX_REQUEST_SIZE. */
  case NBD_CMD_READ:
  case NBD_CMD_WRITE:
    if (count > MAX_REQUEST_SIZE) {
      set_error (ERANGE, "request too large: maximum request size is %d",
                 MAX_REQUEST_SIZE);
      goto err;
    }
    break;

    /* Other commands are currently limited by the 32 bit field in the
     * command structure on the wire, but in future we hope to support
     * 64 bit values here with a change to the NBD protocol which is
     * being discussed upstream.
     */
  default:
    if (count > UINT32_MAX) {
      set_error (ERANGE, "request too large: maximum request size is %" PRIu32,
                 UINT32_MAX);
      goto err;
    }
    break;
  }

  cmd = calloc (1, sizeof *cmd);
  if (cmd == NULL) {
    set_error (errno, "calloc");
    goto err;
  }
  cmd->flags = flags;
  cmd->type = type;
  cmd->cookie = h->unique++;
  cmd->offset = offset;
  cmd->count = count;
  cmd->data = data;
  if (cb)
    cmd->cb = *cb;

  /* If structured replies were negotiated then we trust the server to
   * send back sufficient data to cover the whole buffer.  It's tricky
   * to check this, so an easier thing is simply to zero the buffer
   * ahead of time which avoids any security problems.  I measured the
   * overhead of this and for non-TLS there is no measurable overhead
   * in the highly intensive loopback case.  For TLS we get a
   * performance gain, go figure.
   */
  if (h->structured_replies && cmd->data && type == NBD_CMD_READ)
    memset (cmd->data, 0, cmd->count);

  /* Add the command to the end of the queue. Kick the state machine
   * if there is no other command being processed, otherwise, it will
   * be handled automatically on a future cycle around to READY.
   * Beyond this point, we have to return a cookie to the user, since
   * we are queuing the command, even if kicking the state machine
   * detects a failure.  Not reporting a state machine failure here is
   * okay - any caller of an async command will be calling more API to
   * await results, and will eventually learn that the machine has
   * moved on to DEAD at that time.
   */
  h->in_flight++;
  if (h->cmds_to_issue != NULL) {
    assert (nbd_internal_is_state_processing (get_next_state (h)));
    h->cmds_to_issue_tail = h->cmds_to_issue_tail->next = cmd;
  }
  else {
    assert (h->cmds_to_issue_tail == NULL);
    h->cmds_to_issue = h->cmds_to_issue_tail = cmd;
    if (nbd_internal_is_state_ready (get_next_state (h)) &&
        nbd_internal_run (h, cmd_issue) == -1)
      debug (h, "command queued, ignoring state machine failure");
  }

  return cmd->cookie;

 err:
  /* Since we did not queue the command, we must free the callbacks. */
  if (cb) {
    if (type == NBD_CMD_BLOCK_STATUS)
      FREE_CALLBACK (cb->fn.extent);
    if (type == NBD_CMD_READ)
      FREE_CALLBACK (cb->fn.chunk);
    FREE_CALLBACK (cb->completion);
  }
  return -1;
}