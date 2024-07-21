void ConnectionManagerImpl::ActiveStream::recreateStream(
    StreamInfo::FilterStateSharedPtr filter_state) {
  // n.b. we do not currently change the codecs to point at the new stream
  // decoder because the decoder callbacks are complete. It would be good to
  // null out that pointer but should not be necessary.
  ResponseEncoder* response_encoder = response_encoder_;
  response_encoder_ = nullptr;

  Buffer::InstancePtr request_data = std::make_unique<Buffer::OwnedImpl>();
  const auto& buffered_request_data = filter_manager_.bufferedRequestData();
  const bool proxy_body = buffered_request_data != nullptr && buffered_request_data->length() > 0;
  if (proxy_body) {
    request_data->move(*buffered_request_data);
  }

  response_encoder->getStream().removeCallbacks(*this);
  // This functionally deletes the stream (via deferred delete) so do not
  // reference anything beyond this point.
  connection_manager_.doEndStream(*this);

  RequestDecoder& new_stream = connection_manager_.newStream(*response_encoder, true);
  // We don't need to copy over the old parent FilterState from the old StreamInfo if it did not
  // store any objects with a LifeSpan at or above DownstreamRequest. This is to avoid unnecessary
  // heap allocation.
  // TODO(snowp): In the case where connection level filter state has been set on the connection
  // FilterState that we inherit, we'll end up copying this every time even though we could get
  // away with just resetting it to the HCM filter_state_.
  if (filter_state->hasDataAtOrAboveLifeSpan(StreamInfo::FilterState::LifeSpan::Request)) {
    (*connection_manager_.streams_.begin())->filter_manager_.streamInfo().filter_state_ =
        std::make_shared<StreamInfo::FilterStateImpl>(
            filter_state->parent(), StreamInfo::FilterState::LifeSpan::FilterChain);
  }

  new_stream.decodeHeaders(std::move(request_headers_), !proxy_body);
  if (proxy_body) {
    // This functionality is currently only used for internal redirects, which the router only
    // allows if the full request has been read (end_stream = true) so we don't need to handle the
    // case of upstream sending an early response mid-request.
    new_stream.decodeData(*request_data, true);
  }
}