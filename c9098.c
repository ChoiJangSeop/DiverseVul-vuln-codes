TEST_P(Http2CodecImplTest, HeaderNameWithUnderscoreAllowed) {
  headers_with_underscores_action_ = envoy::config::core::v3::HttpProtocolOptions::ALLOW;
  initialize();

  TestRequestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  request_headers.addCopy("bad_header", "something");
  TestRequestHeaderMapImpl expected_headers(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(HeaderMapEqual(&expected_headers), _));
  EXPECT_CALL(server_stream_callbacks_, onResetStream(_, _)).Times(0);
  request_encoder_->encodeHeaders(request_headers, false);
  EXPECT_EQ(0, stats_store_.counter("http2.dropped_headers_with_underscores").value());
}