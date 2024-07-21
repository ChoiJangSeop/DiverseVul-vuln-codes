void ConnectionHandlerImpl::ActiveTcpSocket::newConnection() {
  // Check if the socket may need to be redirected to another listener.
  ActiveTcpListenerOptRef new_listener;

  if (hand_off_restored_destination_connections_ && socket_->localAddressRestored()) {
    // Find a listener associated with the original destination address.
    new_listener = listener_.parent_.findActiveTcpListenerByAddress(*socket_->localAddress());
  }
  if (new_listener.has_value()) {
    // Hands off connections redirected by iptables to the listener associated with the
    // original destination address. Pass 'hand_off_restored_destination_connections' as false to
    // prevent further redirection as well as 'rebalanced' as true since the connection has
    // already been balanced if applicable inside onAcceptWorker() when the connection was
    // initially accepted. Note also that we must account for the number of connections properly
    // across both listeners.
    // TODO(mattklein123): See note in ~ActiveTcpSocket() related to making this accounting better.
    listener_.decNumConnections();
    new_listener.value().get().incNumConnections();
    new_listener.value().get().onAcceptWorker(std::move(socket_), false, true);
  } else {
    // Set default transport protocol if none of the listener filters did it.
    if (socket_->detectedTransportProtocol().empty()) {
      socket_->setDetectedTransportProtocol(
          Extensions::TransportSockets::TransportProtocolNames::get().RawBuffer);
    }
    // Create a new connection on this listener.
    listener_.newConnection(std::move(socket_));
  }
}