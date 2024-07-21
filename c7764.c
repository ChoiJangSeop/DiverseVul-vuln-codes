int ClientHandler::read_clear() {
  rb_.ensure_chunk();
  for (;;) {
    if (rb_.rleft() && on_read() != 0) {
      return -1;
    }
    if (rb_.rleft() == 0) {
      rb_.reset();
    } else if (rb_.wleft() == 0) {
      conn_.rlimit.stopw();
      return 0;
    }

    if (!ev_is_active(&conn_.rev)) {
      return 0;
    }

    auto nread = conn_.read_clear(rb_.last(), rb_.wleft());

    if (nread == 0) {
      if (rb_.rleft() == 0) {
        rb_.release_chunk();
      }
      return 0;
    }

    if (nread < 0) {
      return -1;
    }

    rb_.write(nread);
  }
}