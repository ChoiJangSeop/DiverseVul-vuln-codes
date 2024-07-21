void ConnectionManagerImpl::doEndStream(ActiveStream& stream) {
  // The order of what happens in this routine is important and a little complicated. We first see
  // if the stream needs to be reset. If it needs to be, this will end up invoking reset callbacks
  // and then moving the stream to the deferred destruction list. If the stream has not been reset,
  // we move it to the deferred deletion list here. Then, we potentially close the connection. This
  // must be done after deleting the stream since the stream refers to the connection and must be
  // deleted first.
  bool reset_stream = false;
  // If the response encoder is still associated with the stream, reset the stream. The exception
  // here is when Envoy "ends" the stream by calling recreateStream at which point recreateStream
  // explicitly nulls out response_encoder to avoid the downstream being notified of the
  // Envoy-internal stream instance being ended.
  if (stream.response_encoder_ != nullptr &&
      (!stream.filter_manager_.remoteComplete() || !stream.state_.codec_saw_local_complete_)) {
    // Indicate local is complete at this point so that if we reset during a continuation, we don't
    // raise further data or trailers.
    ENVOY_STREAM_LOG(debug, "doEndStream() resetting stream", stream);
    // TODO(snowp): This call might not be necessary, try to clean up + remove setter function.
    stream.filter_manager_.setLocalComplete();
    stream.state_.codec_saw_local_complete_ = true;

    // Per https://tools.ietf.org/html/rfc7540#section-8.3 if there was an error
    // with the TCP connection during a CONNECT request, it should be
    // communicated via CONNECT_ERROR
    if (requestWasConnect(stream.request_headers_, codec_->protocol()) &&
        (stream.filter_manager_.streamInfo().hasResponseFlag(
             StreamInfo::ResponseFlag::UpstreamConnectionFailure) ||
         stream.filter_manager_.streamInfo().hasResponseFlag(
             StreamInfo::ResponseFlag::UpstreamConnectionTermination))) {
      stream.response_encoder_->getStream().resetStream(StreamResetReason::ConnectError);
    } else {
      if (stream.filter_manager_.streamInfo().hasResponseFlag(
              StreamInfo::ResponseFlag::UpstreamProtocolError)) {
        stream.response_encoder_->getStream().resetStream(StreamResetReason::ProtocolError);
      } else {
        stream.response_encoder_->getStream().resetStream(StreamResetReason::LocalReset);
      }
    }
    reset_stream = true;
  }

  if (!reset_stream) {
    doDeferredStreamDestroy(stream);
  }

  if (reset_stream && codec_->protocol() < Protocol::Http2) {
    drain_state_ = DrainState::Closing;
  }

  // If HTTP/1.0 has no content length, it is framed by close and won't consider
  // the request complete until the FIN is read. Don't delay close in this case.
  bool http_10_sans_cl = (codec_->protocol() == Protocol::Http10) &&
                         (!stream.response_headers_ || !stream.response_headers_->ContentLength());
  // We also don't delay-close in the case of HTTP/1.1 where the request is
  // fully read, as there's no race condition to avoid.
  bool connection_close = stream.state_.saw_connection_close_;
  bool request_complete = stream.filter_manager_.remoteComplete();

  checkForDeferredClose(connection_close && (request_complete || http_10_sans_cl));
}