void Http1ServerConnectionImplTest::testTrailersExceedLimit(std::string trailer_string,
                                                            bool enable_trailers) {
  initialize();
  // Make a new 'codec' with the right settings
  codec_settings_.enable_trailers_ = enable_trailers;
  codec_ = std::make_unique<ServerConnectionImpl>(
      connection_, http1CodecStats(), callbacks_, codec_settings_, max_request_headers_kb_,
      max_request_headers_count_, envoy::config::core::v3::HttpProtocolOptions::ALLOW);
  std::string exception_reason;
  NiceMock<MockRequestDecoder> decoder;
  EXPECT_CALL(callbacks_, newStream(_, _))
      .WillOnce(Invoke([&](ResponseEncoder&, bool) -> RequestDecoder& { return decoder; }));

  if (enable_trailers) {
    EXPECT_CALL(decoder, decodeHeaders_(_, false));
    EXPECT_CALL(decoder, decodeData(_, false));
  } else {
    EXPECT_CALL(decoder, decodeData(_, false));
    EXPECT_CALL(decoder, decodeData(_, true));
  }

  Buffer::OwnedImpl buffer("POST / HTTP/1.1\r\n"
                           "Host: host\r\n"
                           "Transfer-Encoding: chunked\r\n\r\n"
                           "4\r\n"
                           "body\r\n0\r\n");
  auto status = codec_->dispatch(buffer);
  EXPECT_TRUE(status.ok());
  buffer = Buffer::OwnedImpl(trailer_string + "\r\n\r\n");
  if (enable_trailers) {
    EXPECT_CALL(decoder, sendLocalReply(_, _, _, _, _, _, _));
    status = codec_->dispatch(buffer);
    EXPECT_TRUE(isCodecProtocolError(status));
    EXPECT_EQ(status.message(), "trailers size exceeds limit");
  } else {
    // If trailers are not enabled, we expect Envoy to simply skip over the large
    // trailers as if nothing has happened!
    status = codec_->dispatch(buffer);
    EXPECT_TRUE(status.ok());
  }
}