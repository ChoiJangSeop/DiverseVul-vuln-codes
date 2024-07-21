void ConnectionManagerImpl::ActiveStream::encodeHeaders(ResponseHeaderMap& headers,
                                                        bool end_stream) {
  // Base headers.

  // We want to preserve the original date header, but we add a date header if it is absent
  if (!headers.Date()) {
    connection_manager_.config_.dateProvider().setDateHeader(headers);
  }

  // Following setReference() is safe because serverName() is constant for the life of the
  // listener.
  const auto transformation = connection_manager_.config_.serverHeaderTransformation();
  if (transformation == ConnectionManagerConfig::HttpConnectionManagerProto::OVERWRITE ||
      (transformation == ConnectionManagerConfig::HttpConnectionManagerProto::APPEND_IF_ABSENT &&
       headers.Server() == nullptr)) {
    headers.setReferenceServer(connection_manager_.config_.serverName());
  }
  ConnectionManagerUtility::mutateResponseHeaders(
      headers, request_headers_.get(), connection_manager_.config_,
      connection_manager_.config_.via(), filter_manager_.streamInfo(),
      connection_manager_.proxy_name_, connection_manager_.clear_hop_by_hop_response_headers_);

  bool drain_connection_due_to_overload = false;
  if (connection_manager_.drain_state_ == DrainState::NotDraining &&
      connection_manager_.random_generator_.bernoulli(
          connection_manager_.overload_disable_keepalive_ref_.value())) {
    ENVOY_STREAM_LOG(debug, "disabling keepalive due to envoy overload", *this);
    drain_connection_due_to_overload = true;
    connection_manager_.stats_.named_.downstream_cx_overload_disable_keepalive_.inc();
  }

  // See if we want to drain/close the connection. Send the go away frame prior to encoding the
  // header block.
  if (connection_manager_.drain_state_ == DrainState::NotDraining &&
      (connection_manager_.drain_close_.drainClose() || drain_connection_due_to_overload)) {

    // This doesn't really do anything for HTTP/1.1 other then give the connection another boost
    // of time to race with incoming requests. For HTTP/2 connections, send a GOAWAY frame to
    // prevent any new streams.
    connection_manager_.startDrainSequence();
    connection_manager_.stats_.named_.downstream_cx_drain_close_.inc();
    ENVOY_STREAM_LOG(debug, "drain closing connection", *this);
  }

  if (connection_manager_.codec_->protocol() == Protocol::Http10) {
    // As HTTP/1.0 and below can not do chunked encoding, if there is no content
    // length the response will be framed by connection close.
    if (!headers.ContentLength()) {
      state_.saw_connection_close_ = true;
    }
    // If the request came with a keep-alive and no other factor resulted in a
    // connection close header, send an explicit keep-alive header.
    if (!state_.saw_connection_close_) {
      headers.setConnection(Headers::get().ConnectionValues.KeepAlive);
    }
  }

  if (connection_manager_.drain_state_ == DrainState::NotDraining && state_.saw_connection_close_) {
    ENVOY_STREAM_LOG(debug, "closing connection due to connection close header", *this);
    connection_manager_.drain_state_ = DrainState::Closing;
  }

  // If we are destroying a stream before remote is complete and the connection does not support
  // multiplexing, we should disconnect since we don't want to wait around for the request to
  // finish.
  if (!filter_manager_.remoteComplete()) {
    if (connection_manager_.codec_->protocol() < Protocol::Http2) {
      connection_manager_.drain_state_ = DrainState::Closing;
    }

    connection_manager_.stats_.named_.downstream_rq_response_before_rq_complete_.inc();
  }

  if (connection_manager_.drain_state_ != DrainState::NotDraining &&
      connection_manager_.codec_->protocol() < Protocol::Http2) {
    // If the connection manager is draining send "Connection: Close" on HTTP/1.1 connections.
    // Do not do this for H2 (which drains via GOAWAY) or Upgrade or CONNECT (as the
    // payload is no longer HTTP/1.1)
    if (!Utility::isUpgrade(headers) &&
        !HeaderUtility::isConnectResponse(request_headers_.get(), *responseHeaders())) {
      headers.setReferenceConnection(Headers::get().ConnectionValues.Close);
    }
  }

  if (connection_manager_.config_.tracingConfig()) {
    if (connection_manager_.config_.tracingConfig()->operation_name_ ==
        Tracing::OperationName::Ingress) {
      // For ingress (inbound) responses, if the request headers do not include a
      // decorator operation (override), and the decorated operation should be
      // propagated, then pass the decorator's operation name (if defined)
      // as a response header to enable the client service to use it in its client span.
      if (decorated_operation_ && state_.decorated_propagate_) {
        headers.setEnvoyDecoratorOperation(*decorated_operation_);
      }
    } else if (connection_manager_.config_.tracingConfig()->operation_name_ ==
               Tracing::OperationName::Egress) {
      const HeaderEntry* resp_operation_override = headers.EnvoyDecoratorOperation();

      // For Egress (outbound) response, if a decorator operation name has been provided, it
      // should be used to override the active span's operation.
      if (resp_operation_override) {
        if (!resp_operation_override->value().empty() && active_span_) {
          active_span_->setOperation(resp_operation_override->value().getStringView());
        }
        // Remove header so not propagated to service.
        headers.removeEnvoyDecoratorOperation();
      }
    }
  }

  chargeStats(headers);

  ENVOY_STREAM_LOG(debug, "encoding headers via codec (end_stream={}):\n{}", *this, end_stream,
                   headers);

  // Now actually encode via the codec.
  filter_manager_.streamInfo().downstreamTiming().onFirstDownstreamTxByteSent(
      connection_manager_.time_source_);
  response_encoder_->encodeHeaders(headers, end_stream);
}