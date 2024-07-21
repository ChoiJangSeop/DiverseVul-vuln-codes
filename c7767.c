void test_nghttp2_session_flooding(void) {
  nghttp2_session *session;
  nghttp2_session_callbacks callbacks;
  nghttp2_bufs bufs;
  nghttp2_buf *buf;
  nghttp2_frame frame;
  nghttp2_mem *mem;
  size_t i;

  mem = nghttp2_mem_default();

  frame_pack_bufs_init(&bufs);

  memset(&callbacks, 0, sizeof(callbacks));

  /* PING ACK */
  nghttp2_session_server_new(&session, &callbacks, NULL);

  nghttp2_frame_ping_init(&frame.ping, NGHTTP2_FLAG_NONE, NULL);
  nghttp2_frame_pack_ping(&bufs, &frame.ping);
  nghttp2_frame_ping_free(&frame.ping);

  buf = &bufs.head->buf;

  for (i = 0; i < NGHTTP2_MAX_OBQ_FLOOD_ITEM; ++i) {
    CU_ASSERT(
        (ssize_t)nghttp2_buf_len(buf) ==
        nghttp2_session_mem_recv(session, buf->pos, nghttp2_buf_len(buf)));
  }

  CU_ASSERT(NGHTTP2_ERR_FLOODED ==
            nghttp2_session_mem_recv(session, buf->pos, nghttp2_buf_len(buf)));

  nghttp2_session_del(session);

  /* SETTINGS ACK */
  nghttp2_bufs_reset(&bufs);

  nghttp2_session_server_new(&session, &callbacks, NULL);

  nghttp2_frame_settings_init(&frame.settings, NGHTTP2_FLAG_NONE, NULL, 0);
  nghttp2_frame_pack_settings(&bufs, &frame.settings);
  nghttp2_frame_settings_free(&frame.settings, mem);

  buf = &bufs.head->buf;

  for (i = 0; i < NGHTTP2_MAX_OBQ_FLOOD_ITEM; ++i) {
    CU_ASSERT(
        (ssize_t)nghttp2_buf_len(buf) ==
        nghttp2_session_mem_recv(session, buf->pos, nghttp2_buf_len(buf)));
  }

  CU_ASSERT(NGHTTP2_ERR_FLOODED ==
            nghttp2_session_mem_recv(session, buf->pos, nghttp2_buf_len(buf)));

  nghttp2_session_del(session);
  nghttp2_bufs_free(&bufs);
}