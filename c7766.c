int nghttp2_session_add_settings(nghttp2_session *session, uint8_t flags,
                                 const nghttp2_settings_entry *iv, size_t niv) {
  nghttp2_outbound_item *item;
  nghttp2_frame *frame;
  nghttp2_settings_entry *iv_copy;
  size_t i;
  int rv;
  nghttp2_mem *mem;
  nghttp2_inflight_settings *inflight_settings = NULL;

  mem = &session->mem;

  if (flags & NGHTTP2_FLAG_ACK) {
    if (niv != 0) {
      return NGHTTP2_ERR_INVALID_ARGUMENT;
    }

    if (session->obq_flood_counter_ >= NGHTTP2_MAX_OBQ_FLOOD_ITEM) {
      return NGHTTP2_ERR_FLOODED;
    }
  }

  if (!nghttp2_iv_check(iv, niv)) {
    return NGHTTP2_ERR_INVALID_ARGUMENT;
  }

  item = nghttp2_mem_malloc(mem, sizeof(nghttp2_outbound_item));
  if (item == NULL) {
    return NGHTTP2_ERR_NOMEM;
  }

  if (niv > 0) {
    iv_copy = nghttp2_frame_iv_copy(iv, niv, mem);
    if (iv_copy == NULL) {
      nghttp2_mem_free(mem, item);
      return NGHTTP2_ERR_NOMEM;
    }
  } else {
    iv_copy = NULL;
  }

  if ((flags & NGHTTP2_FLAG_ACK) == 0) {
    rv = inflight_settings_new(&inflight_settings, iv, niv, mem);
    if (rv != 0) {
      assert(nghttp2_is_fatal(rv));
      nghttp2_mem_free(mem, iv_copy);
      nghttp2_mem_free(mem, item);
      return rv;
    }
  }

  nghttp2_outbound_item_init(item);

  frame = &item->frame;

  nghttp2_frame_settings_init(&frame->settings, flags, iv_copy, niv);
  rv = nghttp2_session_add_item(session, item);
  if (rv != 0) {
    /* The only expected error is fatal one */
    assert(nghttp2_is_fatal(rv));

    inflight_settings_del(inflight_settings, mem);

    nghttp2_frame_settings_free(&frame->settings, mem);
    nghttp2_mem_free(mem, item);

    return rv;
  }

  if (flags & NGHTTP2_FLAG_ACK) {
    ++session->obq_flood_counter_;
  } else {
    session_append_inflight_settings(session, inflight_settings);
  }

  /* Extract NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS and ENABLE_PUSH
     here.  We use it to refuse the incoming stream and PUSH_PROMISE
     with RST_STREAM. */

  for (i = niv; i > 0; --i) {
    if (iv[i - 1].settings_id == NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS) {
      session->pending_local_max_concurrent_stream = iv[i - 1].value;
      break;
    }
  }

  for (i = niv; i > 0; --i) {
    if (iv[i - 1].settings_id == NGHTTP2_SETTINGS_ENABLE_PUSH) {
      session->pending_enable_push = (uint8_t)iv[i - 1].value;
      break;
    }
  }

  for (i = niv; i > 0; --i) {
    if (iv[i - 1].settings_id == NGHTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL) {
      session->pending_enable_connect_protocol = (uint8_t)iv[i - 1].value;
      break;
    }
  }

  return 0;
}