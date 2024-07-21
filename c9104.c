void ConnectionImpl::StreamImpl::encodeTrailersBase(const HeaderMap& trailers) {
  ASSERT(!local_end_stream_);
  local_end_stream_ = true;
  if (pending_send_data_.length() > 0) {
    // In this case we want trailers to come after we release all pending body data that is
    // waiting on window updates. We need to save the trailers so that we can emit them later.
    ASSERT(!pending_trailers_to_encode_);
    pending_trailers_to_encode_ = cloneTrailers(trailers);
  } else {
    submitTrailers(trailers);
    parent_.sendPendingFrames();
  }
}