protocol_handshake_oldstyle (struct connection *conn)
{
  struct old_handshake handshake;
  int64_t r;
  uint64_t exportsize;
  uint16_t gflags, eflags;

  /* In --tls=require / FORCEDTLS mode, old style handshakes are
   * rejected because they cannot support TLS.
   */
  if (tls == 2) {
    nbdkit_error ("non-TLS client tried to connect in --tls=require mode");
    return -1;
  }

  r = backend->get_size (backend, conn);
  if (r == -1)
    return -1;
  if (r < 0) {
    nbdkit_error (".get_size function returned invalid value "
                  "(%" PRIi64 ")", r);
    return -1;
  }
  exportsize = (uint64_t) r;
  conn->exportsize = exportsize;

  gflags = 0;
  if (protocol_compute_eflags (conn, &eflags) < 0)
    return -1;

  debug ("oldstyle negotiation: flags: global 0x%x export 0x%x",
         gflags, eflags);

  memset (&handshake, 0, sizeof handshake);
  memcpy (handshake.nbdmagic, "NBDMAGIC", 8);
  handshake.version = htobe64 (OLD_VERSION);
  handshake.exportsize = htobe64 (exportsize);
  handshake.gflags = htobe16 (gflags);
  handshake.eflags = htobe16 (eflags);

  if (conn->send (conn, &handshake, sizeof handshake) == -1) {
    nbdkit_error ("write: %m");
    return -1;
  }

  return 0;
}