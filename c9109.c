TEST_P(Http2CodecImplTest, Invalid204WithContentLengthAllowed) {
  stream_error_on_invalid_http_messaging_ = true;
  initialize();

  MockStreamCallbacks request_callbacks;
  request_encoder_->getStream().addCallbacks(request_callbacks);

  TestRequestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(_, true));
  request_encoder_->encodeHeaders(request_headers, true);

  // Buffer client data to avoid mock recursion causing lifetime issues.
  ON_CALL(server_connection_, write(_, _))
      .WillByDefault(
          Invoke([&](Buffer::Instance& data, bool) -> void { client_wrapper_.buffer_.add(data); }));

  TestResponseHeaderMapImpl response_headers{{":status", "204"}, {"content-length", "3"}};
  // What follows is a hack to get headers that should span into continuation frames. The default
  // maximum frame size is 16K. We will add 3,000 headers that will take us above this size and
  // not easily compress with HPACK. (I confirmed this generates 26,468 bytes of header data
  // which should contain a continuation.)
  for (int i = 1; i < 3000; i++) {
    response_headers.addCopy(std::to_string(i), std::to_string(i));
  }

  response_encoder_->encodeHeaders(response_headers, false);

  // Flush pending data.
  EXPECT_CALL(request_callbacks, onResetStream(StreamResetReason::LocalReset, _));
  EXPECT_CALL(server_stream_callbacks_, onResetStream(StreamResetReason::RemoteReset, _));
  setupDefaultConnectionMocks();
  auto status = client_wrapper_.dispatch(Buffer::OwnedImpl(), *client_);
  EXPECT_TRUE(status.ok());

  EXPECT_EQ(1, stats_store_.counter("http2.rx_messaging_error").value());
  expectDetailsRequest("http2.invalid.header.field");
};