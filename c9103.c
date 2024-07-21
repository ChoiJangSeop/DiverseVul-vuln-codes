int ConnectionImpl::StreamImpl::onDataSourceSend(const uint8_t* framehd, size_t length) {
  // In this callback we are writing out a raw DATA frame without copying. nghttp2 assumes that we
  // "just know" that the frame header is 9 bytes.
  // https://nghttp2.org/documentation/types.html#c.nghttp2_send_data_callback
  static const uint64_t FRAME_HEADER_SIZE = 9;

  parent_.outbound_data_frames_++;

  Buffer::OwnedImpl output;
  if (!parent_.addOutboundFrameFragment(output, framehd, FRAME_HEADER_SIZE)) {
    ENVOY_CONN_LOG(debug, "error sending data frame: Too many frames in the outbound queue",
                   parent_.connection_);
    return NGHTTP2_ERR_FLOODED;
  }

  output.move(pending_send_data_, length);
  parent_.connection_.write(output, false);
  return 0;
}