int Http2Handler::read_tls() {
  std::array<uint8_t, 8_k> buf;

  ERR_clear_error();

  for (;;) {
    auto rv = SSL_read(ssl_, buf.data(), buf.size());

    if (rv <= 0) {
      auto err = SSL_get_error(ssl_, rv);
      switch (err) {
      case SSL_ERROR_WANT_READ:
        goto fin;
      case SSL_ERROR_WANT_WRITE:
        // renegotiation started
        return -1;
      default:
        return -1;
      }
    }

    auto nread = rv;

    if (get_config()->hexdump) {
      util::hexdump(stdout, buf.data(), nread);
    }

    rv = nghttp2_session_mem_recv(session_, buf.data(), nread);
    if (rv < 0) {
      if (rv != NGHTTP2_ERR_BAD_CLIENT_MAGIC) {
        std::cerr << "nghttp2_session_mem_recv() returned error: "
                  << nghttp2_strerror(rv) << std::endl;
      }
      return -1;
    }
  }

fin:
  return write_(*this);
}