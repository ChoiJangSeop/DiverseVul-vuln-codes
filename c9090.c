TEST_P(Http2CodecImplStreamLimitTest, MaxClientStreams) {
  http2OptionsFromTuple(client_http2_options_, ::testing::get<0>(GetParam()));
  http2OptionsFromTuple(server_http2_options_, ::testing::get<1>(GetParam()));
  client_ = std::make_unique<TestClientConnectionImpl>(
      client_connection_, client_callbacks_, stats_store_, client_http2_options_,
      max_request_headers_kb_, max_response_headers_count_, ProdNghttp2SessionFactory::get());
  server_ = std::make_unique<TestServerConnectionImpl>(
      server_connection_, server_callbacks_, stats_store_, server_http2_options_,
      max_request_headers_kb_, max_request_headers_count_, headers_with_underscores_action_);

  for (int i = 0; i < 101; ++i) {
    request_encoder_ = &client_->newStream(response_decoder_);
    setupDefaultConnectionMocks();
    EXPECT_CALL(server_callbacks_, newStream(_, _))
        .WillOnce(Invoke([&](ResponseEncoder& encoder, bool) -> RequestDecoder& {
          response_encoder_ = &encoder;
          encoder.getStream().addCallbacks(server_stream_callbacks_);
          return request_decoder_;
        }));

    TestRequestHeaderMapImpl request_headers;
    HttpTestUtility::addDefaultHeaders(request_headers);
    EXPECT_CALL(request_decoder_, decodeHeaders_(_, true));
    request_encoder_->encodeHeaders(request_headers, true);
  }
}