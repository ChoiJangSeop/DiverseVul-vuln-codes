ConnectionImpl::ConnectionImpl(Network::Connection& connection, CodecStats& stats,
                               const envoy::config::core::v3::Http2ProtocolOptions& http2_options,
                               const uint32_t max_headers_kb, const uint32_t max_headers_count)
    : stats_(stats), connection_(connection), max_headers_kb_(max_headers_kb),
      max_headers_count_(max_headers_count),
      per_stream_buffer_limit_(http2_options.initial_stream_window_size().value()),
      stream_error_on_invalid_http_messaging_(
          http2_options.stream_error_on_invalid_http_messaging()),
      flood_detected_(false), max_outbound_frames_(http2_options.max_outbound_frames().value()),
      frame_buffer_releasor_([this](const Buffer::OwnedBufferFragmentImpl* fragment) {
        releaseOutboundFrame(fragment);
      }),
      max_outbound_control_frames_(http2_options.max_outbound_control_frames().value()),
      control_frame_buffer_releasor_([this](const Buffer::OwnedBufferFragmentImpl* fragment) {
        releaseOutboundControlFrame(fragment);
      }),
      max_consecutive_inbound_frames_with_empty_payload_(
          http2_options.max_consecutive_inbound_frames_with_empty_payload().value()),
      max_inbound_priority_frames_per_stream_(
          http2_options.max_inbound_priority_frames_per_stream().value()),
      max_inbound_window_update_frames_per_data_frame_sent_(
          http2_options.max_inbound_window_update_frames_per_data_frame_sent().value()),
      dispatching_(false), raised_goaway_(false), pending_deferred_reset_(false) {}