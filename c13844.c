void ConnPoolImplBase::onConnectionEvent(ActiveClient& client, absl::string_view failure_reason,
                                         Network::ConnectionEvent event) {
  if (client.state() == ActiveClient::State::CONNECTING) {
    ASSERT(connecting_stream_capacity_ >= client.effectiveConcurrentStreamLimit());
    connecting_stream_capacity_ -= client.effectiveConcurrentStreamLimit();
  }

  if (client.connect_timer_) {
    client.connect_timer_->disableTimer();
    client.connect_timer_.reset();
  }

  if (event == Network::ConnectionEvent::RemoteClose ||
      event == Network::ConnectionEvent::LocalClose) {
    state_.decrConnectingAndConnectedStreamCapacity(client.currentUnusedCapacity());
    // Make sure that onStreamClosed won't double count.
    client.remaining_streams_ = 0;
    // The client died.
    ENVOY_CONN_LOG(debug, "client disconnected, failure reason: {}", client, failure_reason);

    Envoy::Upstream::reportUpstreamCxDestroy(host_, event);
    const bool incomplete_stream = client.closingWithIncompleteStream();
    if (incomplete_stream) {
      Envoy::Upstream::reportUpstreamCxDestroyActiveRequest(host_, event);
    }

    if (client.state() == ActiveClient::State::CONNECTING) {
      host_->cluster().stats().upstream_cx_connect_fail_.inc();
      host_->stats().cx_connect_fail_.inc();

      ConnectionPool::PoolFailureReason reason;
      if (client.timed_out_) {
        reason = ConnectionPool::PoolFailureReason::Timeout;
      } else if (event == Network::ConnectionEvent::RemoteClose) {
        reason = ConnectionPool::PoolFailureReason::RemoteConnectionFailure;
      } else {
        reason = ConnectionPool::PoolFailureReason::LocalConnectionFailure;
      }

      // Raw connect failures should never happen under normal circumstances. If we have an upstream
      // that is behaving badly, streams can get stuck here in the pending state. If we see a
      // connect failure, we purge all pending streams so that calling code can determine what to
      // do with the stream.
      // NOTE: We move the existing pending streams to a temporary list. This is done so that
      //       if retry logic submits a new stream to the pool, we don't fail it inline.
      purgePendingStreams(client.real_host_description_, failure_reason, reason);
      // See if we should preconnect based on active connections.
      if (!is_draining_for_deletion_) {
        tryCreateNewConnections();
      }
    }

    // We need to release our resourceManager() resources before checking below for
    // whether we can create a new connection. Normally this would happen when
    // client's destructor runs, but this object needs to be deferredDelete'd(), so
    // this forces part of its cleanup to happen now.
    client.releaseResources();

    // Again, since we know this object is going to be deferredDelete'd(), we take
    // this opportunity to disable and reset the connection duration timer so that
    // it doesn't trigger while on the deferred delete list. In theory it is safe
    // to handle the CLOSED state in onConnectionDurationTimeout, but we handle
    // it here for simplicity and safety anyway.
    if (client.connection_duration_timer_) {
      client.connection_duration_timer_->disableTimer();
      client.connection_duration_timer_.reset();
    }

    dispatcher_.deferredDelete(client.removeFromList(owningList(client.state())));

    checkForIdleAndCloseIdleConnsIfDraining();

    client.setState(ActiveClient::State::CLOSED);

    // If we have pending streams and we just lost a connection we should make a new one.
    if (!pending_streams_.empty()) {
      tryCreateNewConnections();
    }
  } else if (event == Network::ConnectionEvent::Connected) {
    client.conn_connect_ms_->complete();
    client.conn_connect_ms_.reset();
    ASSERT(client.state() == ActiveClient::State::CONNECTING);
    bool streams_available = client.currentUnusedCapacity() > 0;
    transitionActiveClientState(client, streams_available ? ActiveClient::State::READY
                                                          : ActiveClient::State::BUSY);

    // Now that the active client is ready, set up a timer for max connection duration.
    const absl::optional<std::chrono::milliseconds> max_connection_duration =
        client.parent_.host()->cluster().maxConnectionDuration();
    if (max_connection_duration.has_value()) {
      client.connection_duration_timer_ = client.parent_.dispatcher().createTimer(
          [&client]() { client.onConnectionDurationTimeout(); });
      client.connection_duration_timer_->enableTimer(max_connection_duration.value());
    }

    // At this point, for the mixed ALPN pool, the client may be deleted. Do not
    // refer to client after this point.
    onConnected(client);
    if (streams_available) {
      onUpstreamReady();
    }
    checkForIdleAndCloseIdleConnsIfDraining();
  }
}