CodecClient::CodecClient(Type type, Network::ClientConnectionPtr&& connection,
                         Upstream::HostDescriptionConstSharedPtr host,
                         Event::Dispatcher& dispatcher)
    : type_(type), connection_(std::move(connection)), host_(host),
      idle_timeout_(host_->cluster().idleTimeout()) {
  if (type_ != Type::HTTP3) {
    // Make sure upstream connections process data and then the FIN, rather than processing
    // TCP disconnects immediately. (see https://github.com/envoyproxy/envoy/issues/1679 for
    // details)
    connection_->detectEarlyCloseWhenReadDisabled(false);
  }
  connection_->addConnectionCallbacks(*this);
  connection_->addReadFilter(Network::ReadFilterSharedPtr{new CodecReadFilter(*this)});

  ENVOY_CONN_LOG(debug, "connecting", *connection_);
  connection_->connect();

  if (idle_timeout_) {
    idle_timer_ = dispatcher.createTimer([this]() -> void { onIdleTimeout(); });
    enableIdleTimer();
  }

  // We just universally set no delay on connections. Theoretically we might at some point want
  // to make this configurable.
  connection_->noDelay(true);
}