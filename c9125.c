void ServerConnectionImpl::onUrl(const char* data, size_t length) {
  if (active_request_.has_value()) {
    active_request_.value().request_url_.append(data, length);
  }
}