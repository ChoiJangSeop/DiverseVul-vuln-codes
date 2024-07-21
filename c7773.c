void Downstream::inspect_http1_request() {
  if (req_.method == HTTP_CONNECT) {
    req_.upgrade_request = true;
  } else if (req_.http_minor > 0) {
    auto upgrade = req_.fs.header(http2::HD_UPGRADE);
    if (upgrade) {
      const auto &val = upgrade->value;
      // TODO Perform more strict checking for upgrade headers
      if (util::streq_l(NGHTTP2_CLEARTEXT_PROTO_VERSION_ID, val.c_str(),
                        val.size())) {
        req_.http2_upgrade_seen = true;
      } else {
        req_.upgrade_request = true;

        // TODO Should we check Sec-WebSocket-Key, and
        // Sec-WebSocket-Version as well?
        if (util::strieq_l("websocket", val)) {
          req_.connect_proto = ConnectProto::WEBSOCKET;
        }
      }
    }
  }
  auto transfer_encoding = req_.fs.header(http2::HD_TRANSFER_ENCODING);
  if (transfer_encoding) {
    req_.fs.content_length = -1;
    if (util::iends_with_l(transfer_encoding->value, "chunked")) {
      chunked_request_ = true;
    }
  }
}