int HttpDownstreamConnection::write_first() {
  int rv;

  process_blocked_request_buf();

  if (conn_.tls.ssl) {
    rv = write_tls();
  } else {
    rv = write_clear();
  }

  if (rv != 0) {
    return SHRPX_ERR_RETRY;
  }

  if (conn_.tls.ssl) {
    on_write_ = &HttpDownstreamConnection::write_tls;
  } else {
    on_write_ = &HttpDownstreamConnection::write_clear;
  }

  first_write_done_ = true;
  downstream_->set_request_header_sent(true);

  auto buf = downstream_->get_blocked_request_buf();
  buf->reset();

  return 0;
}