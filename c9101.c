ConnectionImpl::StreamImpl::StreamImpl(ConnectionImpl& parent, uint32_t buffer_limit)
    : parent_(parent), local_end_stream_sent_(false), remote_end_stream_(false),
      data_deferred_(false), waiting_for_non_informational_headers_(false),
      pending_receive_buffer_high_watermark_called_(false),
      pending_send_buffer_high_watermark_called_(false), reset_due_to_messaging_error_(false) {
  if (buffer_limit > 0) {
    setWriteBufferWatermarks(buffer_limit / 2, buffer_limit);
  }
}