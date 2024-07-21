Http2Options::Http2Options(Environment* env) {
  nghttp2_option_new(&options_);

  // We manually handle flow control within a session in order to
  // implement backpressure -- that is, we only send WINDOW_UPDATE
  // frames to the remote peer as data is actually consumed by user
  // code. This ensures that the flow of data over the connection
  // does not move too quickly and limits the amount of data we
  // are required to buffer.
  nghttp2_option_set_no_auto_window_update(options_, 1);

  AliasedBuffer<uint32_t, v8::Uint32Array>& buffer =
      env->http2_state()->options_buffer;
  uint32_t flags = buffer[IDX_OPTIONS_FLAGS];

  if (flags & (1 << IDX_OPTIONS_MAX_DEFLATE_DYNAMIC_TABLE_SIZE)) {
    nghttp2_option_set_max_deflate_dynamic_table_size(
        options_,
        buffer[IDX_OPTIONS_MAX_DEFLATE_DYNAMIC_TABLE_SIZE]);
  }

  if (flags & (1 << IDX_OPTIONS_MAX_RESERVED_REMOTE_STREAMS)) {
    nghttp2_option_set_max_reserved_remote_streams(
        options_,
        buffer[IDX_OPTIONS_MAX_RESERVED_REMOTE_STREAMS]);
  }

  if (flags & (1 << IDX_OPTIONS_MAX_SEND_HEADER_BLOCK_LENGTH)) {
    nghttp2_option_set_max_send_header_block_length(
        options_,
        buffer[IDX_OPTIONS_MAX_SEND_HEADER_BLOCK_LENGTH]);
  }

  // Recommended default
  nghttp2_option_set_peer_max_concurrent_streams(options_, 100);
  if (flags & (1 << IDX_OPTIONS_PEER_MAX_CONCURRENT_STREAMS)) {
    nghttp2_option_set_peer_max_concurrent_streams(
        options_,
        buffer[IDX_OPTIONS_PEER_MAX_CONCURRENT_STREAMS]);
  }

  // The padding strategy sets the mechanism by which we determine how much
  // additional frame padding to apply to DATA and HEADERS frames. Currently
  // this is set on a per-session basis, but eventually we may switch to
  // a per-stream setting, giving users greater control
  if (flags & (1 << IDX_OPTIONS_PADDING_STRATEGY)) {
    padding_strategy_type strategy =
        static_cast<padding_strategy_type>(
            buffer.GetValue(IDX_OPTIONS_PADDING_STRATEGY));
    SetPaddingStrategy(strategy);
  }

  // The max header list pairs option controls the maximum number of
  // header pairs the session may accept. This is a hard limit.. that is,
  // if the remote peer sends more than this amount, the stream will be
  // automatically closed with an RST_STREAM.
  if (flags & (1 << IDX_OPTIONS_MAX_HEADER_LIST_PAIRS)) {
    SetMaxHeaderPairs(buffer[IDX_OPTIONS_MAX_HEADER_LIST_PAIRS]);
  }

  // The HTTP2 specification places no limits on the number of HTTP2
  // PING frames that can be sent. In order to prevent PINGS from being
  // abused as an attack vector, however, we place a strict upper limit
  // on the number of unacknowledged PINGS that can be sent at any given
  // time.
  if (flags & (1 << IDX_OPTIONS_MAX_OUTSTANDING_PINGS)) {
    SetMaxOutstandingPings(buffer[IDX_OPTIONS_MAX_OUTSTANDING_PINGS]);
  }

  // The HTTP2 specification places no limits on the number of HTTP2
  // SETTINGS frames that can be sent. In order to prevent PINGS from being
  // abused as an attack vector, however, we place a strict upper limit
  // on the number of unacknowledged SETTINGS that can be sent at any given
  // time.
  if (flags & (1 << IDX_OPTIONS_MAX_OUTSTANDING_SETTINGS)) {
    SetMaxOutstandingSettings(buffer[IDX_OPTIONS_MAX_OUTSTANDING_SETTINGS]);
  }
}