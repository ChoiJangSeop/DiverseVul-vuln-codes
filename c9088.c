void ConnectionImpl::StreamImpl::encodeData(Buffer::Instance& data, bool end_stream) {
  ASSERT(!local_end_stream_);
  local_end_stream_ = end_stream;
  pending_send_data_.move(data);
  if (data_deferred_) {
    int rc = nghttp2_session_resume_data(parent_.session_, stream_id_);
    ASSERT(rc == 0);

    data_deferred_ = false;
  }

  parent_.sendPendingFrames();
}