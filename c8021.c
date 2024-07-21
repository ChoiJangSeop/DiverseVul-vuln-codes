protocol_compute_eflags (struct connection *conn, uint16_t *flags)
{
  uint16_t eflags = NBD_FLAG_HAS_FLAGS;
  int fl;

  fl = backend->can_write (backend, conn);
  if (fl == -1)
    return -1;
  if (readonly || !fl) {
    eflags |= NBD_FLAG_READ_ONLY;
    conn->readonly = true;
  }
  if (!conn->readonly) {
    fl = backend->can_zero (backend, conn);
    if (fl == -1)
      return -1;
    if (fl) {
      eflags |= NBD_FLAG_SEND_WRITE_ZEROES;
      conn->can_zero = true;
    }

    fl = backend->can_trim (backend, conn);
    if (fl == -1)
      return -1;
    if (fl) {
      eflags |= NBD_FLAG_SEND_TRIM;
      conn->can_trim = true;
    }

    fl = backend->can_fua (backend, conn);
    if (fl == -1)
      return -1;
    if (fl) {
      eflags |= NBD_FLAG_SEND_FUA;
      conn->can_fua = true;
    }
  }

  fl = backend->can_flush (backend, conn);
  if (fl == -1)
    return -1;
  if (fl) {
    eflags |= NBD_FLAG_SEND_FLUSH;
    conn->can_flush = true;
  }

  fl = backend->is_rotational (backend, conn);
  if (fl == -1)
    return -1;
  if (fl) {
    eflags |= NBD_FLAG_ROTATIONAL;
    conn->is_rotational = true;
  }

  /* multi-conn is useless if parallel connections are not allowed */
  if (backend->thread_model (backend) >
      NBDKIT_THREAD_MODEL_SERIALIZE_CONNECTIONS) {
    fl = backend->can_multi_conn (backend, conn);
    if (fl == -1)
      return -1;
    if (fl) {
      eflags |= NBD_FLAG_CAN_MULTI_CONN;
      conn->can_multi_conn = true;
    }
  }

  /* The result of this is not returned to callers here (or at any
   * time during the handshake).  However it makes sense to do it once
   * per connection and store the result in the handle anyway.  This
   * protocol_compute_eflags function is a bit misnamed XXX.
   */
  fl = backend->can_extents (backend, conn);
  if (fl == -1)
    return -1;
  if (fl)
    conn->can_extents = true;

  if (conn->structured_replies)
    eflags |= NBD_FLAG_SEND_DF;

  *flags = eflags;
  return 0;
}