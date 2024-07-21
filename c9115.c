TEST_P(Http2CodecImplDeferredResetTest, DeferredResetServer) {
  initialize();

  InSequence s;

  TestRequestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(_, false));
  request_encoder_->encodeHeaders(request_headers, false);

  // In this case we do the same thing as DeferredResetClient but on the server side.
  ON_CALL(server_connection_, write(_, _))
      .WillByDefault(
          Invoke([&](Buffer::Instance& data, bool) -> void { client_wrapper_.buffer_.add(data); }));
  TestResponseHeaderMapImpl response_headers{{":status", "200"}};
  response_encoder_->encodeHeaders(response_headers, false);
  Buffer::OwnedImpl body(std::string(1024 * 1024, 'a'));
  EXPECT_CALL(server_stream_callbacks_, onAboveWriteBufferHighWatermark()).Times(AnyNumber());
  response_encoder_->encodeData(body, true);
  EXPECT_CALL(server_stream_callbacks_, onResetStream(StreamResetReason::LocalReset, _));
  response_encoder_->getStream().resetStream(StreamResetReason::LocalReset);

  MockStreamCallbacks client_stream_callbacks;
  request_encoder_->getStream().addCallbacks(client_stream_callbacks);
  EXPECT_CALL(response_decoder_, decodeHeaders_(_, false));
  EXPECT_CALL(response_decoder_, decodeData(_, false)).Times(AtLeast(1));
  EXPECT_CALL(client_stream_callbacks, onResetStream(StreamResetReason::RemoteReset, _));
  setupDefaultConnectionMocks();
  auto status = client_wrapper_.dispatch(Buffer::OwnedImpl(), *client_);
  EXPECT_TRUE(status.ok());
}