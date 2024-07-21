int ConnectionImpl::onStreamClose(int32_t stream_id, uint32_t error_code) {
  StreamImpl* stream = getStream(stream_id);
  if (stream) {
    ENVOY_CONN_LOG(debug, "stream closed: {}", connection_, error_code);
    if (!stream->remote_end_stream_ || !stream->local_end_stream_) {
      StreamResetReason reason;
      if (stream->reset_due_to_messaging_error_) {
        // Unfortunately, the nghttp2 API makes it incredibly difficult to clearly understand
        // the flow of resets. I.e., did the reset originate locally? Was it remote? Here,
        // we attempt to track cases in which we sent a reset locally due to an invalid frame
        // received from the remote. We only do that in two cases currently (HTTP messaging layer
        // errors from https://tools.ietf.org/html/rfc7540#section-8 which nghttp2 is very strict
        // about). In other cases we treat invalid frames as a protocol error and just kill
        // the connection.
        reason = StreamResetReason::LocalReset;
      } else {
        reason = error_code == NGHTTP2_REFUSED_STREAM ? StreamResetReason::RemoteRefusedStreamReset
                                                      : StreamResetReason::RemoteReset;
      }

      stream->runResetCallbacks(reason);
    }

    connection_.dispatcher().deferredDelete(stream->removeFromList(active_streams_));
    // Any unconsumed data must be consumed before the stream is deleted.
    // nghttp2 does not appear to track this internally, and any stream deleted
    // with outstanding window will contribute to a slow connection-window leak.
    nghttp2_session_consume(session_, stream_id, stream->unconsumed_bytes_);
    stream->unconsumed_bytes_ = 0;
    nghttp2_session_set_stream_user_data(session_, stream->stream_id_, nullptr);
  }

  return 0;
}