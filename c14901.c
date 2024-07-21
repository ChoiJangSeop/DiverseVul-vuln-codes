void Filter::onDownstreamEvent(Network::ConnectionEvent event) {
  ENVOY_CONN_LOG(trace, "on downstream event {}, has upstream = {}", read_callbacks_->connection(),
                 static_cast<int>(event), upstream_ == nullptr);
  if (upstream_) {
    Tcp::ConnectionPool::ConnectionDataPtr conn_data(upstream_->onDownstreamEvent(event));
    if (conn_data != nullptr &&
        conn_data->connection().state() != Network::Connection::State::Closed) {
      config_->drainManager().add(config_->sharedConfig(), std::move(conn_data),
                                  std::move(upstream_callbacks_), std::move(idle_timer_),
                                  read_callbacks_->upstreamHost());
    }
    if (event != Network::ConnectionEvent::Connected) {
      upstream_.reset();
      disableIdleTimer();
    }
  }
  if (generic_conn_pool_) {
    if (event == Network::ConnectionEvent::LocalClose ||
        event == Network::ConnectionEvent::RemoteClose) {
      // Cancel the conn pool request and close any excess pending requests.
      generic_conn_pool_.reset();
    }
  }
}