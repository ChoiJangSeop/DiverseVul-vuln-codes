void Filter::onUpstreamEvent(Network::ConnectionEvent event) {
  // Update the connecting flag before processing the event because we may start a new connection
  // attempt in initializeUpstreamConnection.
  bool connecting = connecting_;
  connecting_ = false;

  if (event == Network::ConnectionEvent::RemoteClose ||
      event == Network::ConnectionEvent::LocalClose) {
    upstream_.reset();
    disableIdleTimer();

    if (connecting) {
      if (event == Network::ConnectionEvent::RemoteClose) {
        getStreamInfo().setResponseFlag(StreamInfo::ResponseFlag::UpstreamConnectionFailure);
        read_callbacks_->upstreamHost()->outlierDetector().putResult(
            Upstream::Outlier::Result::LocalOriginConnectFailed);
      }

      initializeUpstreamConnection();
    } else {
      if (read_callbacks_->connection().state() == Network::Connection::State::Open) {
        read_callbacks_->connection().close(Network::ConnectionCloseType::FlushWrite);
      }
    }
  }
}