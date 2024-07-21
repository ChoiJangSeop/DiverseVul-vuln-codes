TEST_F(Http1ClientConnectionImplTest, LargeResponseHeadersRejected) {
  initialize();

  NiceMock<MockRequestDecoder> decoder;
  NiceMock<MockResponseDecoder> response_decoder;
  Http::RequestEncoder& request_encoder = codec_->newStream(response_decoder);
  TestRequestHeaderMapImpl headers{{":method", "GET"}, {":path", "/"}, {":authority", "host"}};
  request_encoder.encodeHeaders(headers, true);

  Buffer::OwnedImpl buffer("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n");
  auto status = codec_->dispatch(buffer);
  EXPECT_TRUE(status.ok());
  std::string long_header = "big: " + std::string(80 * 1024, 'q') + "\r\n";
  buffer = Buffer::OwnedImpl(long_header);
  status = codec_->dispatch(buffer);
  EXPECT_TRUE(isCodecProtocolError(status));
  EXPECT_EQ(status.message(), "headers size exceeds limit");
}