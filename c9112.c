TEST_P(Http2CodecImplTest, HeaderNameWithUnderscoreAreRejectedByDefault) {
  headers_with_underscores_action_ = envoy::config::core::v3::HttpProtocolOptions::REJECT_REQUEST;
  initialize();

  TestRequestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  request_headers.addCopy("bad_header", "something");
  EXPECT_CALL(server_stream_callbacks_, onResetStream(_, _)).Times(1);
  request_encoder_->encodeHeaders(request_headers, false);
  EXPECT_EQ(1, stats_store_.counter("http2.requests_rejected_with_underscores_in_headers").value());
}