static int session_new(nghttp2_session **session_ptr,
                       const nghttp2_session_callbacks *callbacks,
                       void *user_data, int server,
                       const nghttp2_option *option, nghttp2_mem *mem) {
  int rv;
  size_t nbuffer;
  size_t max_deflate_dynamic_table_size =
      NGHTTP2_HD_DEFAULT_MAX_DEFLATE_BUFFER_SIZE;

  if (mem == NULL) {
    mem = nghttp2_mem_default();
  }

  *session_ptr = nghttp2_mem_calloc(mem, 1, sizeof(nghttp2_session));
  if (*session_ptr == NULL) {
    rv = NGHTTP2_ERR_NOMEM;
    goto fail_session;
  }

  (*session_ptr)->mem = *mem;
  mem = &(*session_ptr)->mem;

  /* next_stream_id is initialized in either
     nghttp2_session_client_new2 or nghttp2_session_server_new2 */

  nghttp2_stream_init(&(*session_ptr)->root, 0, NGHTTP2_STREAM_FLAG_NONE,
                      NGHTTP2_STREAM_IDLE, NGHTTP2_DEFAULT_WEIGHT, 0, 0, NULL,
                      mem);

  (*session_ptr)->remote_window_size = NGHTTP2_INITIAL_CONNECTION_WINDOW_SIZE;
  (*session_ptr)->recv_window_size = 0;
  (*session_ptr)->consumed_size = 0;
  (*session_ptr)->recv_reduction = 0;
  (*session_ptr)->local_window_size = NGHTTP2_INITIAL_CONNECTION_WINDOW_SIZE;

  (*session_ptr)->goaway_flags = NGHTTP2_GOAWAY_NONE;
  (*session_ptr)->local_last_stream_id = (1u << 31) - 1;
  (*session_ptr)->remote_last_stream_id = (1u << 31) - 1;

  (*session_ptr)->pending_local_max_concurrent_stream =
      NGHTTP2_DEFAULT_MAX_CONCURRENT_STREAMS;
  (*session_ptr)->pending_enable_push = 1;

  if (server) {
    (*session_ptr)->server = 1;
  }

  init_settings(&(*session_ptr)->remote_settings);
  init_settings(&(*session_ptr)->local_settings);

  (*session_ptr)->max_incoming_reserved_streams =
      NGHTTP2_MAX_INCOMING_RESERVED_STREAMS;

  /* Limit max outgoing concurrent streams to sensible value */
  (*session_ptr)->remote_settings.max_concurrent_streams = 100;

  (*session_ptr)->max_send_header_block_length = NGHTTP2_MAX_HEADERSLEN;
  (*session_ptr)->max_outbound_ack = NGHTTP2_DEFAULT_MAX_OBQ_FLOOD_ITEM;

  if (option) {
    if ((option->opt_set_mask & NGHTTP2_OPT_NO_AUTO_WINDOW_UPDATE) &&
        option->no_auto_window_update) {

      (*session_ptr)->opt_flags |= NGHTTP2_OPTMASK_NO_AUTO_WINDOW_UPDATE;
    }

    if (option->opt_set_mask & NGHTTP2_OPT_PEER_MAX_CONCURRENT_STREAMS) {

      (*session_ptr)->remote_settings.max_concurrent_streams =
          option->peer_max_concurrent_streams;
    }

    if (option->opt_set_mask & NGHTTP2_OPT_MAX_RESERVED_REMOTE_STREAMS) {

      (*session_ptr)->max_incoming_reserved_streams =
          option->max_reserved_remote_streams;
    }

    if ((option->opt_set_mask & NGHTTP2_OPT_NO_RECV_CLIENT_MAGIC) &&
        option->no_recv_client_magic) {

      (*session_ptr)->opt_flags |= NGHTTP2_OPTMASK_NO_RECV_CLIENT_MAGIC;
    }

    if ((option->opt_set_mask & NGHTTP2_OPT_NO_HTTP_MESSAGING) &&
        option->no_http_messaging) {

      (*session_ptr)->opt_flags |= NGHTTP2_OPTMASK_NO_HTTP_MESSAGING;
    }

    if (option->opt_set_mask & NGHTTP2_OPT_USER_RECV_EXT_TYPES) {
      memcpy((*session_ptr)->user_recv_ext_types, option->user_recv_ext_types,
             sizeof((*session_ptr)->user_recv_ext_types));
    }

    if (option->opt_set_mask & NGHTTP2_OPT_BUILTIN_RECV_EXT_TYPES) {
      (*session_ptr)->builtin_recv_ext_types = option->builtin_recv_ext_types;
    }

    if ((option->opt_set_mask & NGHTTP2_OPT_NO_AUTO_PING_ACK) &&
        option->no_auto_ping_ack) {
      (*session_ptr)->opt_flags |= NGHTTP2_OPTMASK_NO_AUTO_PING_ACK;
    }

    if (option->opt_set_mask & NGHTTP2_OPT_MAX_SEND_HEADER_BLOCK_LENGTH) {
      (*session_ptr)->max_send_header_block_length =
          option->max_send_header_block_length;
    }

    if (option->opt_set_mask & NGHTTP2_OPT_MAX_DEFLATE_DYNAMIC_TABLE_SIZE) {
      max_deflate_dynamic_table_size = option->max_deflate_dynamic_table_size;
    }

    if ((option->opt_set_mask & NGHTTP2_OPT_NO_CLOSED_STREAMS) &&
        option->no_closed_streams) {
      (*session_ptr)->opt_flags |= NGHTTP2_OPTMASK_NO_CLOSED_STREAMS;
    }

    if (option->opt_set_mask & NGHTTP2_OPT_MAX_OUTBOUND_ACK) {
      (*session_ptr)->max_outbound_ack = option->max_outbound_ack;
    }
  }

  rv = nghttp2_hd_deflate_init2(&(*session_ptr)->hd_deflater,
                                max_deflate_dynamic_table_size, mem);
  if (rv != 0) {
    goto fail_hd_deflater;
  }
  rv = nghttp2_hd_inflate_init(&(*session_ptr)->hd_inflater, mem);
  if (rv != 0) {
    goto fail_hd_inflater;
  }
  rv = nghttp2_map_init(&(*session_ptr)->streams, mem);
  if (rv != 0) {
    goto fail_map;
  }

  nbuffer = ((*session_ptr)->max_send_header_block_length +
             NGHTTP2_FRAMEBUF_CHUNKLEN - 1) /
            NGHTTP2_FRAMEBUF_CHUNKLEN;

  if (nbuffer == 0) {
    nbuffer = 1;
  }

  /* 1 for Pad Field. */
  rv = nghttp2_bufs_init3(&(*session_ptr)->aob.framebufs,
                          NGHTTP2_FRAMEBUF_CHUNKLEN, nbuffer, 1,
                          NGHTTP2_FRAME_HDLEN + 1, mem);
  if (rv != 0) {
    goto fail_aob_framebuf;
  }

  active_outbound_item_reset(&(*session_ptr)->aob, mem);

  (*session_ptr)->callbacks = *callbacks;
  (*session_ptr)->user_data = user_data;

  session_inbound_frame_reset(*session_ptr);

  if (nghttp2_enable_strict_preface) {
    nghttp2_inbound_frame *iframe = &(*session_ptr)->iframe;

    if (server && ((*session_ptr)->opt_flags &
                   NGHTTP2_OPTMASK_NO_RECV_CLIENT_MAGIC) == 0) {
      iframe->state = NGHTTP2_IB_READ_CLIENT_MAGIC;
      iframe->payloadleft = NGHTTP2_CLIENT_MAGIC_LEN;
    } else {
      iframe->state = NGHTTP2_IB_READ_FIRST_SETTINGS;
    }

    if (!server) {
      (*session_ptr)->aob.state = NGHTTP2_OB_SEND_CLIENT_MAGIC;
      nghttp2_bufs_add(&(*session_ptr)->aob.framebufs, NGHTTP2_CLIENT_MAGIC,
                       NGHTTP2_CLIENT_MAGIC_LEN);
    }
  }

  return 0;

fail_aob_framebuf:
  nghttp2_map_free(&(*session_ptr)->streams);
fail_map:
  nghttp2_hd_inflate_free(&(*session_ptr)->hd_inflater);
fail_hd_inflater:
  nghttp2_hd_deflate_free(&(*session_ptr)->hd_deflater);
fail_hd_deflater:
  nghttp2_mem_free(mem, *session_ptr);
fail_session:
  return rv;
}