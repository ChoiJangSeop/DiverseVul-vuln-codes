void ConnectionManagerImpl::ActiveStream::decodeHeaders(RequestHeaderMapPtr&& headers,
                                                        bool end_stream) {
  ScopeTrackerScopeState scope(this,
                               connection_manager_.read_callbacks_->connection().dispatcher());
  request_headers_ = std::move(headers);

  // Both saw_connection_close_ and is_head_request_ affect local replies: set
  // them as early as possible.
  const Protocol protocol = connection_manager_.codec_->protocol();
  const bool fixed_connection_close =
      Runtime::runtimeFeatureEnabled("envoy.reloadable_features.fixed_connection_close");
  if (fixed_connection_close) {
    state_.saw_connection_close_ =
        HeaderUtility::shouldCloseConnection(protocol, *request_headers_);
  }
  if (Http::Headers::get().MethodValues.Head == request_headers_->getMethodValue()) {
    state_.is_head_request_ = true;
  }

  if (HeaderUtility::isConnect(*request_headers_) && !request_headers_->Path() &&
      !Runtime::runtimeFeatureEnabled("envoy.reloadable_features.stop_faking_paths")) {
    request_headers_->setPath("/");
  }

  // We need to snap snapped_route_config_ here as it's used in mutateRequestHeaders later.
  if (connection_manager_.config_.isRoutable()) {
    if (connection_manager_.config_.routeConfigProvider() != nullptr) {
      snapped_route_config_ = connection_manager_.config_.routeConfigProvider()->config();
    } else if (connection_manager_.config_.scopedRouteConfigProvider() != nullptr) {
      snapped_scoped_routes_config_ =
          connection_manager_.config_.scopedRouteConfigProvider()->config<Router::ScopedConfig>();
      snapScopedRouteConfig();
    }
  } else {
    snapped_route_config_ = connection_manager_.config_.routeConfigProvider()->config();
  }

  ENVOY_STREAM_LOG(debug, "request headers complete (end_stream={}):\n{}", *this, end_stream,
                   *request_headers_);

  // We end the decode here only if the request is header only. If we convert the request to a
  // header only, the stream will be marked as done once a subsequent decodeData/decodeTrailers is
  // called with end_stream=true.
  maybeEndDecode(end_stream);

  // Drop new requests when overloaded as soon as we have decoded the headers.
  if (connection_manager_.overload_stop_accepting_requests_ref_ ==
      Server::OverloadActionState::Active) {
    // In this one special case, do not create the filter chain. If there is a risk of memory
    // overload it is more important to avoid unnecessary allocation than to create the filters.
    state_.created_filter_chain_ = true;
    connection_manager_.stats_.named_.downstream_rq_overload_close_.inc();
    sendLocalReply(Grpc::Common::isGrpcRequestHeaders(*request_headers_),
                   Http::Code::ServiceUnavailable, "envoy overloaded", nullptr,
                   state_.is_head_request_, absl::nullopt,
                   StreamInfo::ResponseCodeDetails::get().Overload);
    return;
  }

  if (!connection_manager_.config_.proxy100Continue() && request_headers_->Expect() &&
      request_headers_->Expect()->value() == Headers::get().ExpectValues._100Continue.c_str()) {
    // Note in the case Envoy is handling 100-Continue complexity, it skips the filter chain
    // and sends the 100-Continue directly to the encoder.
    chargeStats(continueHeader());
    response_encoder_->encode100ContinueHeaders(continueHeader());
    // Remove the Expect header so it won't be handled again upstream.
    request_headers_->removeExpect();
  }

  connection_manager_.user_agent_.initializeFromHeaders(*request_headers_,
                                                        connection_manager_.stats_.prefixStatName(),
                                                        connection_manager_.stats_.scope_);

  // Make sure we are getting a codec version we support.
  if (protocol == Protocol::Http10) {
    // Assume this is HTTP/1.0. This is fine for HTTP/0.9 but this code will also affect any
    // requests with non-standard version numbers (0.9, 1.3), basically anything which is not
    // HTTP/1.1.
    //
    // The protocol may have shifted in the HTTP/1.0 case so reset it.
    stream_info_.protocol(protocol);
    if (!connection_manager_.config_.http1Settings().accept_http_10_) {
      // Send "Upgrade Required" if HTTP/1.0 support is not explicitly configured on.
      sendLocalReply(false, Code::UpgradeRequired, "", nullptr, state_.is_head_request_,
                     absl::nullopt, StreamInfo::ResponseCodeDetails::get().LowVersion);
      return;
    } else if (!fixed_connection_close) {
      // HTTP/1.0 defaults to single-use connections. Make sure the connection
      // will be closed unless Keep-Alive is present.
      state_.saw_connection_close_ = true;
      if (absl::EqualsIgnoreCase(request_headers_->getConnectionValue(),
                                 Http::Headers::get().ConnectionValues.KeepAlive)) {
        state_.saw_connection_close_ = false;
      }
    }
    if (!request_headers_->Host() &&
        !connection_manager_.config_.http1Settings().default_host_for_http_10_.empty()) {
      // Add a default host if configured to do so.
      request_headers_->setHost(
          connection_manager_.config_.http1Settings().default_host_for_http_10_);
    }
  }

  if (!request_headers_->Host()) {
    // Require host header. For HTTP/1.1 Host has already been translated to :authority.
    sendLocalReply(Grpc::Common::hasGrpcContentType(*request_headers_), Code::BadRequest, "",
                   nullptr, state_.is_head_request_, absl::nullopt,
                   StreamInfo::ResponseCodeDetails::get().MissingHost);
    return;
  }

  // Verify header sanity checks which should have been performed by the codec.
  ASSERT(HeaderUtility::requestHeadersValid(*request_headers_).has_value() == false);

  // Check for the existence of the :path header for non-CONNECT requests, or present-but-empty
  // :path header for CONNECT requests. We expect the codec to have broken the path into pieces if
  // applicable. NOTE: Currently the HTTP/1.1 codec only does this when the allow_absolute_url flag
  // is enabled on the HCM.
  if ((!HeaderUtility::isConnect(*request_headers_) || request_headers_->Path()) &&
      request_headers_->getPathValue().empty()) {
    sendLocalReply(Grpc::Common::hasGrpcContentType(*request_headers_), Code::NotFound, "", nullptr,
                   state_.is_head_request_, absl::nullopt,
                   StreamInfo::ResponseCodeDetails::get().MissingPath);
    return;
  }

  // Currently we only support relative paths at the application layer.
  if (!request_headers_->getPathValue().empty() && request_headers_->getPathValue()[0] != '/') {
    connection_manager_.stats_.named_.downstream_rq_non_relative_path_.inc();
    sendLocalReply(Grpc::Common::hasGrpcContentType(*request_headers_), Code::NotFound, "", nullptr,
                   state_.is_head_request_, absl::nullopt,
                   StreamInfo::ResponseCodeDetails::get().AbsolutePath);
    return;
  }

  // Path sanitization should happen before any path access other than the above sanity check.
  if (!ConnectionManagerUtility::maybeNormalizePath(*request_headers_,
                                                    connection_manager_.config_)) {
    sendLocalReply(Grpc::Common::hasGrpcContentType(*request_headers_), Code::BadRequest, "",
                   nullptr, state_.is_head_request_, absl::nullopt,
                   StreamInfo::ResponseCodeDetails::get().PathNormalizationFailed);
    return;
  }

  ConnectionManagerUtility::maybeNormalizeHost(*request_headers_, connection_manager_.config_,
                                               localPort());

  if (!fixed_connection_close && protocol == Protocol::Http11 &&
      absl::EqualsIgnoreCase(request_headers_->getConnectionValue(),
                             Http::Headers::get().ConnectionValues.Close)) {
    state_.saw_connection_close_ = true;
  }
  // Note: Proxy-Connection is not a standard header, but is supported here
  // since it is supported by http-parser the underlying parser for http
  // requests.
  if (!fixed_connection_close && protocol < Protocol::Http2 && !state_.saw_connection_close_ &&
      absl::EqualsIgnoreCase(request_headers_->getProxyConnectionValue(),
                             Http::Headers::get().ConnectionValues.Close)) {
    state_.saw_connection_close_ = true;
  }

  if (!state_.is_internally_created_) { // Only sanitize headers on first pass.
    // Modify the downstream remote address depending on configuration and headers.
    stream_info_.setDownstreamRemoteAddress(ConnectionManagerUtility::mutateRequestHeaders(
        *request_headers_, connection_manager_.read_callbacks_->connection(),
        connection_manager_.config_, *snapped_route_config_, connection_manager_.local_info_));
  }
  ASSERT(stream_info_.downstreamRemoteAddress() != nullptr);

  ASSERT(!cached_route_);
  refreshCachedRoute();

  if (!state_.is_internally_created_) { // Only mutate tracing headers on first pass.
    ConnectionManagerUtility::mutateTracingRequestHeader(
        *request_headers_, connection_manager_.runtime_, connection_manager_.config_,
        cached_route_.value().get());
  }

  stream_info_.setRequestHeaders(*request_headers_);

  const bool upgrade_rejected = createFilterChain() == false;

  // TODO if there are no filters when starting a filter iteration, the connection manager
  // should return 404. The current returns no response if there is no router filter.
  if (hasCachedRoute()) {
    // Do not allow upgrades if the route does not support it.
    if (upgrade_rejected) {
      // While downstream servers should not send upgrade payload without the upgrade being
      // accepted, err on the side of caution and refuse to process any further requests on this
      // connection, to avoid a class of HTTP/1.1 smuggling bugs where Upgrade or CONNECT payload
      // contains a smuggled HTTP request.
      state_.saw_connection_close_ = true;
      connection_manager_.stats_.named_.downstream_rq_ws_on_non_ws_route_.inc();
      sendLocalReply(Grpc::Common::hasGrpcContentType(*request_headers_), Code::Forbidden, "",
                     nullptr, state_.is_head_request_, absl::nullopt,
                     StreamInfo::ResponseCodeDetails::get().UpgradeFailed);
      return;
    }
    // Allow non websocket requests to go through websocket enabled routes.
  }

  if (hasCachedRoute()) {
    const Router::RouteEntry* route_entry = cached_route_.value()->routeEntry();
    if (route_entry != nullptr && route_entry->idleTimeout()) {
      idle_timeout_ms_ = route_entry->idleTimeout().value();
      if (idle_timeout_ms_.count()) {
        // If we have a route-level idle timeout but no global stream idle timeout, create a timer.
        if (stream_idle_timer_ == nullptr) {
          stream_idle_timer_ =
              connection_manager_.read_callbacks_->connection().dispatcher().createTimer(
                  [this]() -> void { onIdleTimeout(); });
        }
      } else if (stream_idle_timer_ != nullptr) {
        // If we had a global stream idle timeout but the route-level idle timeout is set to zero
        // (to override), we disable the idle timer.
        stream_idle_timer_->disableTimer();
        stream_idle_timer_ = nullptr;
      }
    }
  }

  // Check if tracing is enabled at all.
  if (connection_manager_.config_.tracingConfig()) {
    traceRequest();
  }

  decodeHeaders(nullptr, *request_headers_, end_stream);

  // Reset it here for both global and overridden cases.
  resetIdleTimer();
}