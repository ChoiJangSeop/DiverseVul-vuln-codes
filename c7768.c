int nghttp2_session_add_ping(nghttp2_session *session, uint8_t flags,
                             const uint8_t *opaque_data) {
  int rv;
  nghttp2_outbound_item *item;
  nghttp2_frame *frame;
  nghttp2_mem *mem;

  mem = &session->mem;

  if ((flags & NGHTTP2_FLAG_ACK) &&
      session->obq_flood_counter_ >= NGHTTP2_MAX_OBQ_FLOOD_ITEM) {
    return NGHTTP2_ERR_FLOODED;
  }

  item = nghttp2_mem_malloc(mem, sizeof(nghttp2_outbound_item));
  if (item == NULL) {
    return NGHTTP2_ERR_NOMEM;
  }

  nghttp2_outbound_item_init(item);

  frame = &item->frame;

  nghttp2_frame_ping_init(&frame->ping, flags, opaque_data);

  rv = nghttp2_session_add_item(session, item);

  if (rv != 0) {
    nghttp2_frame_ping_free(&frame->ping);
    nghttp2_mem_free(mem, item);
    return rv;
  }

  if (flags & NGHTTP2_FLAG_ACK) {
    ++session->obq_flood_counter_;
  }

  return 0;
}