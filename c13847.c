void ConnPoolImplBase::closeIdleConnectionsForDrainingPool() {
  // Create a separate list of elements to close to avoid mutate-while-iterating problems.
  std::list<ActiveClient*> to_close;

  for (auto& client : ready_clients_) {
    if (client->numActiveStreams() == 0) {
      to_close.push_back(client.get());
    }
  }

  if (pending_streams_.empty()) {
    for (auto& client : connecting_clients_) {
      to_close.push_back(client.get());
    }
  }

  for (auto& entry : to_close) {
    ENVOY_LOG_EVENT(debug, "closing_idle_client", "closing idle client {} for cluster {}",
                    entry->id(), host_->cluster().name());
    entry->close();
  }
}