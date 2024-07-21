RequestDecoder& ConnectionManagerImpl::newStream(ResponseEncoder& response_encoder,
                                                 bool is_internally_created) {
  if (connection_idle_timer_) {
    connection_idle_timer_->disableTimer();
  }

  ENVOY_CONN_LOG(debug, "new stream", read_callbacks_->connection());
  ActiveStreamPtr new_stream(new ActiveStream(*this));
  new_stream->state_.is_internally_created_ = is_internally_created;
  new_stream->response_encoder_ = &response_encoder;
  new_stream->response_encoder_->getStream().addCallbacks(*new_stream);
  new_stream->buffer_limit_ = new_stream->response_encoder_->getStream().bufferLimit();
  // If the network connection is backed up, the stream should be made aware of it on creation.
  // Both HTTP/1.x and HTTP/2 codecs handle this in StreamCallbackHelper::addCallbacksHelper.
  ASSERT(read_callbacks_->connection().aboveHighWatermark() == false ||
         new_stream->high_watermark_count_ > 0);
  new_stream->moveIntoList(std::move(new_stream), streams_);
  return **streams_.begin();
}