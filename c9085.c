TEST_P(Http2CodecImplTest, InvalidContinueWithFin) {
  initialize();

  TestRequestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(_, true));
  request_encoder_->encodeHeaders(request_headers, true);

  TestResponseHeaderMapImpl continue_headers{{":status", "100"}};
  EXPECT_THROW(response_encoder_->encodeHeaders(continue_headers, true), ClientCodecError);
  EXPECT_EQ(1, stats_store_.counter("http2.rx_messaging_error").value());
}