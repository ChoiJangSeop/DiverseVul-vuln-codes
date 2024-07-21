TEST_P(Http2CodecImplTest, Invalid103) {
  initialize();

  TestRequestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(_, true));
  request_encoder_->encodeHeaders(request_headers, true);

  TestResponseHeaderMapImpl continue_headers{{":status", "100"}};
  EXPECT_CALL(response_decoder_, decode100ContinueHeaders_(_));
  response_encoder_->encode100ContinueHeaders(continue_headers);

  TestResponseHeaderMapImpl early_hint_headers{{":status", "103"}};
  EXPECT_CALL(response_decoder_, decodeHeaders_(_, false));
  response_encoder_->encodeHeaders(early_hint_headers, false);

  EXPECT_THROW_WITH_MESSAGE(response_encoder_->encodeHeaders(early_hint_headers, false),
                            ClientCodecError, "Unexpected 'trailers' with no end stream.");
  EXPECT_EQ(1, stats_store_.counter("http2.too_many_header_frames").value());
}