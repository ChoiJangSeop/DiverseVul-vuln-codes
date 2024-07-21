static int nghttp2_session_upgrade_internal(nghttp2_session *session,
                                            const uint8_t *settings_payload,
                                            size_t settings_payloadlen,
                                            void *stream_user_data) {
  nghttp2_stream *stream;
  nghttp2_frame frame;
  nghttp2_settings_entry *iv;
  size_t niv;
  int rv;
  nghttp2_priority_spec pri_spec;
  nghttp2_mem *mem;

  mem = &session->mem;

  if ((!session->server && session->next_stream_id != 1) ||
      (session->server && session->last_recv_stream_id >= 1)) {
    return NGHTTP2_ERR_PROTO;
  }
  if (settings_payloadlen % NGHTTP2_FRAME_SETTINGS_ENTRY_LENGTH) {
    return NGHTTP2_ERR_INVALID_ARGUMENT;
  }
  rv = nghttp2_frame_unpack_settings_payload2(&iv, &niv, settings_payload,
                                              settings_payloadlen, mem);
  if (rv != 0) {
    return rv;
  }

  if (session->server) {
    nghttp2_frame_hd_init(&frame.hd, settings_payloadlen, NGHTTP2_SETTINGS,
                          NGHTTP2_FLAG_NONE, 0);
    frame.settings.iv = iv;
    frame.settings.niv = niv;
    rv = nghttp2_session_on_settings_received(session, &frame, 1 /* No ACK */);
  } else {
    rv = nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, iv, niv);
  }
  nghttp2_mem_free(mem, iv);
  if (rv != 0) {
    return rv;
  }

  nghttp2_priority_spec_default_init(&pri_spec);

  stream = nghttp2_session_open_stream(
      session, 1, NGHTTP2_STREAM_FLAG_NONE, &pri_spec, NGHTTP2_STREAM_OPENING,
      session->server ? NULL : stream_user_data);
  if (stream == NULL) {
    return NGHTTP2_ERR_NOMEM;
  }

  /* We don't call nghttp2_session_adjust_closed_stream(), since this
     should be the first stream open. */

  if (session->server) {
    nghttp2_stream_shutdown(stream, NGHTTP2_SHUT_RD);
    session->last_recv_stream_id = 1;
    session->last_proc_stream_id = 1;
  } else {
    nghttp2_stream_shutdown(stream, NGHTTP2_SHUT_WR);
    session->last_sent_stream_id = 1;
    session->next_stream_id += 2;
  }
  return 0;
}