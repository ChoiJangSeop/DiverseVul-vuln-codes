int Http2Handler::read_clear() {
  int rv;
  std::array<uint8_t, 8_k> buf;

  for (;;) {
    ssize_t nread;
    while ((nread = read(fd_, buf.data(), buf.size())) == -1 && errno == EINTR)
      ;
    if (nread == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      return -1;
    }
    if (nread == 0) {
      return -1;
    }

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

  return write_(*this);
}