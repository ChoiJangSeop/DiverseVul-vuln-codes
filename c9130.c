void ConnectionHandlerImpl::ActiveTcpListener::onAccept(Network::ConnectionSocketPtr&& socket) {
  onAcceptWorker(std::move(socket), config_->handOffRestoredDestinationConnections(), false);
}