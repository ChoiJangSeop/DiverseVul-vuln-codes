void ServerConnectionImpl::onResetStream(StreamResetReason reason) {
  active_request_->response_encoder_.runResetCallbacks(reason);
  connection_.dispatcher().deferredDelete(std::move(active_request_));
}