int HttpDownstreamConnection::push_request_headers() {
  if (request_header_written_) {
    signal_write();
    return 0;
  }

  const auto &downstream_hostport = addr_->hostport;
  const auto &req = downstream_->request();

  auto &balloc = downstream_->get_block_allocator();

  auto connect_method = req.regular_connect_method();

  auto config = get_config();
  auto &httpconf = config->http;

  request_header_written_ = true;

  // For HTTP/1.0 request, there is no authority in request.  In that
  // case, we use backend server's host nonetheless.
  auto authority = StringRef(downstream_hostport);
  auto no_host_rewrite =
      httpconf.no_host_rewrite || config->http2_proxy || connect_method;

  if (no_host_rewrite && !req.authority.empty()) {
    authority = req.authority;
  }

  downstream_->set_request_downstream_host(authority);

  auto buf = downstream_->get_request_buf();

  // Assume that method and request path do not contain \r\n.
  auto meth = http2::to_method_string(
      req.connect_proto == ConnectProto::WEBSOCKET ? HTTP_GET : req.method);
  buf->append(meth);
  buf->append(' ');

  if (connect_method) {
    buf->append(authority);
  } else if (config->http2_proxy) {
    // Construct absolute-form request target because we are going to
    // send a request to a HTTP/1 proxy.
    assert(!req.scheme.empty());
    buf->append(req.scheme);
    buf->append("://");
    buf->append(authority);
    buf->append(req.path);
  } else if (req.method == HTTP_OPTIONS && req.path.empty()) {
    // Server-wide OPTIONS
    buf->append("*");
  } else {
    buf->append(req.path);
  }
  buf->append(" HTTP/1.1\r\nHost: ");
  buf->append(authority);
  buf->append("\r\n");

  auto &fwdconf = httpconf.forwarded;
  auto &xffconf = httpconf.xff;
  auto &xfpconf = httpconf.xfp;
  auto &earlydataconf = httpconf.early_data;

  uint32_t build_flags =
      (fwdconf.strip_incoming ? http2::HDOP_STRIP_FORWARDED : 0) |
      (xffconf.strip_incoming ? http2::HDOP_STRIP_X_FORWARDED_FOR : 0) |
      (xfpconf.strip_incoming ? http2::HDOP_STRIP_X_FORWARDED_PROTO : 0) |
      (earlydataconf.strip_incoming ? http2::HDOP_STRIP_EARLY_DATA : 0) |
      (req.http_major == 2 ? http2::HDOP_STRIP_SEC_WEBSOCKET_KEY : 0);

  http2::build_http1_headers_from_headers(buf, req.fs.headers(), build_flags);

  auto cookie = downstream_->assemble_request_cookie();
  if (!cookie.empty()) {
    buf->append("Cookie: ");
    buf->append(cookie);
    buf->append("\r\n");
  }

  // set transfer-encoding only when content-length is unknown and
  // request body is expected.
  if (req.method != HTTP_CONNECT && req.http2_expect_body &&
      req.fs.content_length == -1) {
    downstream_->set_chunked_request(true);
    buf->append("Transfer-Encoding: chunked\r\n");
  }

  if (req.connect_proto == ConnectProto::WEBSOCKET) {
    if (req.http_major == 2) {
      std::array<uint8_t, 16> nonce;
      util::random_bytes(std::begin(nonce), std::end(nonce),
                         worker_->get_randgen());
      auto iov = make_byte_ref(balloc, base64::encode_length(nonce.size()) + 1);
      auto p = base64::encode(std::begin(nonce), std::end(nonce), iov.base);
      *p = '\0';
      auto key = StringRef{iov.base, p};
      downstream_->set_ws_key(key);

      buf->append("Sec-Websocket-Key: ");
      buf->append(key);
      buf->append("\r\n");
    }

    buf->append("Upgrade: websocket\r\nConnection: Upgrade\r\n");
  } else if (!connect_method && req.upgrade_request) {
    auto connection = req.fs.header(http2::HD_CONNECTION);
    if (connection) {
      buf->append("Connection: ");
      buf->append((*connection).value);
      buf->append("\r\n");
    }

    auto upgrade = req.fs.header(http2::HD_UPGRADE);
    if (upgrade) {
      buf->append("Upgrade: ");
      buf->append((*upgrade).value);
      buf->append("\r\n");
    }
  } else if (req.connection_close) {
    buf->append("Connection: close\r\n");
  }

  auto upstream = downstream_->get_upstream();
  auto handler = upstream->get_client_handler();

#if OPENSSL_1_1_1_API
  auto conn = handler->get_connection();

  if (conn->tls.ssl && !SSL_is_init_finished(conn->tls.ssl)) {
    buf->append("Early-Data: 1\r\n");
  }
#endif // OPENSSL_1_1_1_API

  auto fwd =
      fwdconf.strip_incoming ? nullptr : req.fs.header(http2::HD_FORWARDED);

  if (fwdconf.params) {
    auto params = fwdconf.params;

    if (config->http2_proxy || connect_method) {
      params &= ~FORWARDED_PROTO;
    }

    auto value = http::create_forwarded(
        balloc, params, handler->get_forwarded_by(),
        handler->get_forwarded_for(), req.authority, req.scheme);

    if (fwd || !value.empty()) {
      buf->append("Forwarded: ");
      if (fwd) {
        buf->append(fwd->value);

        if (!value.empty()) {
          buf->append(", ");
        }
      }
      buf->append(value);
      buf->append("\r\n");
    }
  } else if (fwd) {
    buf->append("Forwarded: ");
    buf->append(fwd->value);
    buf->append("\r\n");
  }

  auto xff = xffconf.strip_incoming ? nullptr
                                    : req.fs.header(http2::HD_X_FORWARDED_FOR);

  if (xffconf.add) {
    buf->append("X-Forwarded-For: ");
    if (xff) {
      buf->append((*xff).value);
      buf->append(", ");
    }
    buf->append(client_handler_->get_ipaddr());
    buf->append("\r\n");
  } else if (xff) {
    buf->append("X-Forwarded-For: ");
    buf->append((*xff).value);
    buf->append("\r\n");
  }
  if (!config->http2_proxy && !connect_method) {
    auto xfp = xfpconf.strip_incoming
                   ? nullptr
                   : req.fs.header(http2::HD_X_FORWARDED_PROTO);

    if (xfpconf.add) {
      buf->append("X-Forwarded-Proto: ");
      if (xfp) {
        buf->append((*xfp).value);
        buf->append(", ");
      }
      assert(!req.scheme.empty());
      buf->append(req.scheme);
      buf->append("\r\n");
    } else if (xfp) {
      buf->append("X-Forwarded-Proto: ");
      buf->append((*xfp).value);
      buf->append("\r\n");
    }
  }
  auto via = req.fs.header(http2::HD_VIA);
  if (httpconf.no_via) {
    if (via) {
      buf->append("Via: ");
      buf->append((*via).value);
      buf->append("\r\n");
    }
  } else {
    buf->append("Via: ");
    if (via) {
      buf->append((*via).value);
      buf->append(", ");
    }
    std::array<char, 16> viabuf;
    auto end = http::create_via_header_value(viabuf.data(), req.http_major,
                                             req.http_minor);
    buf->append(viabuf.data(), end - viabuf.data());
    buf->append("\r\n");
  }

  for (auto &p : httpconf.add_request_headers) {
    buf->append(p.name);
    buf->append(": ");
    buf->append(p.value);
    buf->append("\r\n");
  }

  buf->append("\r\n");

  if (LOG_ENABLED(INFO)) {
    std::string nhdrs;
    for (auto chunk = buf->head; chunk; chunk = chunk->next) {
      nhdrs.append(chunk->pos, chunk->last);
    }
    if (log_config()->errorlog_tty) {
      nhdrs = http::colorizeHeaders(nhdrs.c_str());
    }
    DCLOG(INFO, this) << "HTTP request headers. stream_id="
                      << downstream_->get_stream_id() << "\n"
                      << nhdrs;
  }

  // Don't call signal_write() if we anticipate request body.  We call
  // signal_write() when we received request body chunk, and it
  // enables us to send headers and data in one writev system call.
  if (req.method == HTTP_CONNECT ||
      downstream_->get_blocked_request_buf()->rleft() ||
      (!req.http2_expect_body && req.fs.content_length == 0)) {
    signal_write();
  }

  return 0;
}