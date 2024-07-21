int htp_hdrs_completecb(llhttp_t *htp) {
  int rv;
  auto upstream = static_cast<HttpsUpstream *>(htp->data);
  if (LOG_ENABLED(INFO)) {
    ULOG(INFO, upstream) << "HTTP request headers completed";
  }

  auto handler = upstream->get_client_handler();

  auto downstream = upstream->get_downstream();
  auto &req = downstream->request();

  auto lgconf = log_config();
  lgconf->update_tstamp(std::chrono::system_clock::now());
  req.tstamp = lgconf->tstamp;

  req.http_major = htp->http_major;
  req.http_minor = htp->http_minor;

  req.connection_close = !llhttp_should_keep_alive(htp);

  handler->stop_read_timer();

  auto method = req.method;

  if (LOG_ENABLED(INFO)) {
    std::stringstream ss;
    ss << http2::to_method_string(method) << " "
       << (method == HTTP_CONNECT ? req.authority : req.path) << " "
       << "HTTP/" << req.http_major << "." << req.http_minor << "\n";

    for (const auto &kv : req.fs.headers()) {
      if (kv.name == "authorization") {
        ss << TTY_HTTP_HD << kv.name << TTY_RST << ": <redacted>\n";
        continue;
      }
      ss << TTY_HTTP_HD << kv.name << TTY_RST << ": " << kv.value << "\n";
    }

    ULOG(INFO, upstream) << "HTTP request headers\n" << ss.str();
  }

  // set content-length if method is not CONNECT, and no
  // transfer-encoding is given.  If transfer-encoding is given, leave
  // req.fs.content_length to -1.
  if (method != HTTP_CONNECT && !req.fs.header(http2::HD_TRANSFER_ENCODING)) {
    // llhttp sets 0 to htp->content_length if there is no
    // content-length header field.  If we don't have both
    // transfer-encoding and content-length header field, we assume
    // that there is no request body.
    req.fs.content_length = htp->content_length;
  }

  auto host = req.fs.header(http2::HD_HOST);

  if (req.http_major > 1 || req.http_minor > 1) {
    req.http_major = 1;
    req.http_minor = 1;
    return -1;
  }

  if (req.http_major == 1 && req.http_minor == 1 && !host) {
    return -1;
  }

  if (host) {
    const auto &value = host->value;
    // Not allow at least '"' or '\' in host.  They are illegal in
    // authority component, also they cause headaches when we put them
    // in quoted-string.
    if (std::find_if(std::begin(value), std::end(value), [](char c) {
          return c == '"' || c == '\\';
        }) != std::end(value)) {
      return -1;
    }
  }

  downstream->inspect_http1_request();

  auto faddr = handler->get_upstream_addr();
  auto &balloc = downstream->get_block_allocator();
  auto config = get_config();

  if (method != HTTP_CONNECT) {
    http_parser_url u{};
    rv = http_parser_parse_url(req.path.c_str(), req.path.size(), 0, &u);
    if (rv != 0) {
      // Expect to respond with 400 bad request
      return -1;
    }
    // checking UF_HOST could be redundant, but just in case ...
    if (!(u.field_set & (1 << UF_SCHEMA)) || !(u.field_set & (1 << UF_HOST))) {
      req.no_authority = true;

      if (method == HTTP_OPTIONS && req.path == StringRef::from_lit("*")) {
        req.path = StringRef{};
      } else {
        req.path = http2::rewrite_clean_path(balloc, req.path);
      }

      if (host) {
        req.authority = host->value;
      }

      if (handler->get_ssl()) {
        req.scheme = StringRef::from_lit("https");
      } else {
        req.scheme = StringRef::from_lit("http");
      }
    } else {
      rewrite_request_host_path_from_uri(balloc, req, req.path, u);
    }
  }

  downstream->set_request_state(DownstreamState::HEADER_COMPLETE);

#ifdef HAVE_MRUBY
  auto worker = handler->get_worker();
  auto mruby_ctx = worker->get_mruby_context();

  auto &resp = downstream->response();

  if (mruby_ctx->run_on_request_proc(downstream) != 0) {
    resp.http_status = 500;
    return -1;
  }
#endif // HAVE_MRUBY

  // mruby hook may change method value

  if (req.no_authority && config->http2_proxy &&
      faddr->alt_mode == UpstreamAltMode::NONE) {
    // Request URI should be absolute-form for client proxy mode
    return -1;
  }

  if (downstream->get_response_state() == DownstreamState::MSG_COMPLETE) {
    return 0;
  }

  DownstreamConnection *dconn_ptr;

  for (;;) {
    auto dconn = handler->get_downstream_connection(rv, downstream);

    if (!dconn) {
      if (rv == SHRPX_ERR_TLS_REQUIRED) {
        upstream->redirect_to_https(downstream);
      }
      downstream->set_request_state(DownstreamState::CONNECT_FAIL);
      return -1;
    }

#ifdef HAVE_MRUBY
    dconn_ptr = dconn.get();
#endif // HAVE_MRUBY
    if (downstream->attach_downstream_connection(std::move(dconn)) == 0) {
      break;
    }
  }

#ifdef HAVE_MRUBY
  const auto &group = dconn_ptr->get_downstream_addr_group();
  if (group) {
    const auto &dmruby_ctx = group->mruby_ctx;

    if (dmruby_ctx->run_on_request_proc(downstream) != 0) {
      resp.http_status = 500;
      return -1;
    }

    if (downstream->get_response_state() == DownstreamState::MSG_COMPLETE) {
      return 0;
    }
  }
#endif // HAVE_MRUBY

  rv = downstream->push_request_headers();

  if (rv != 0) {
    return -1;
  }

  if (faddr->alt_mode != UpstreamAltMode::NONE) {
    // Normally, we forward expect: 100-continue to backend server,
    // and let them decide whether responds with 100 Continue or not.
    // For alternative mode, we have no backend, so just send 100
    // Continue here to make the client happy.
    auto expect = req.fs.header(http2::HD_EXPECT);
    if (expect &&
        util::strieq(expect->value, StringRef::from_lit("100-continue"))) {
      auto output = downstream->get_response_buf();
      constexpr auto res = StringRef::from_lit("HTTP/1.1 100 Continue\r\n\r\n");
      output->append(res);
      handler->signal_write();
    }
  }

  return 0;
}