TEST_P(Http2CodecImplFlowControlTest, TestFlowControlInPendingSendData) {
  initialize();
  MockStreamCallbacks callbacks;
  request_encoder_->getStream().addCallbacks(callbacks);

  TestRequestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  TestRequestHeaderMapImpl expected_headers;
  HttpTestUtility::addDefaultHeaders(expected_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(HeaderMapEqual(&expected_headers), false));
  request_encoder_->encodeHeaders(request_headers, false);

  // Force the server stream to be read disabled. This will cause it to stop sending window
  // updates to the client.
  server_->getStream(1)->readDisable(true);

  uint32_t initial_stream_window =
      nghttp2_session_get_stream_effective_local_window_size(client_->session(), 1);
  // If this limit is changed, this test will fail due to the initial large writes being divided
  // into more than 4 frames. Fast fail here with this explanatory comment.
  ASSERT_EQ(65535, initial_stream_window);
  // Make sure the limits were configured properly in test set up.
  EXPECT_EQ(initial_stream_window, server_->getStream(1)->bufferLimit());
  EXPECT_EQ(initial_stream_window, client_->getStream(1)->bufferLimit());

  // One large write gets broken into smaller frames.
  EXPECT_CALL(request_decoder_, decodeData(_, false)).Times(AnyNumber());
  Buffer::OwnedImpl long_data(std::string(initial_stream_window, 'a'));
  request_encoder_->encodeData(long_data, false);

  // Verify that the window is full. The client will not send more data to the server for this
  // stream.
  EXPECT_EQ(0, nghttp2_session_get_stream_local_window_size(server_->session(), 1));
  EXPECT_EQ(0, nghttp2_session_get_stream_remote_window_size(client_->session(), 1));
  EXPECT_EQ(initial_stream_window, server_->getStream(1)->unconsumed_bytes_);

  // Now that the flow control window is full, further data causes the send buffer to back up.
  Buffer::OwnedImpl more_long_data(std::string(initial_stream_window, 'a'));
  request_encoder_->encodeData(more_long_data, false);
  EXPECT_EQ(initial_stream_window, client_->getStream(1)->pending_send_data_.length());
  EXPECT_EQ(initial_stream_window, server_->getStream(1)->unconsumed_bytes_);

  // If we go over the limit, the stream callbacks should fire.
  EXPECT_CALL(callbacks, onAboveWriteBufferHighWatermark());
  Buffer::OwnedImpl last_byte("!");
  request_encoder_->encodeData(last_byte, false);
  EXPECT_EQ(initial_stream_window + 1, client_->getStream(1)->pending_send_data_.length());

  // Now create a second stream on the connection.
  MockResponseDecoder response_decoder2;
  RequestEncoder* request_encoder2 = &client_->newStream(response_decoder_);
  StreamEncoder* response_encoder2;
  MockStreamCallbacks server_stream_callbacks2;
  MockRequestDecoder request_decoder2;
  // When the server stream is created it should check the status of the
  // underlying connection. Pretend it is overrun.
  EXPECT_CALL(server_connection_, aboveHighWatermark()).WillOnce(Return(true));
  EXPECT_CALL(server_stream_callbacks2, onAboveWriteBufferHighWatermark());
  EXPECT_CALL(server_callbacks_, newStream(_, _))
      .WillOnce(Invoke([&](ResponseEncoder& encoder, bool) -> RequestDecoder& {
        response_encoder2 = &encoder;
        encoder.getStream().addCallbacks(server_stream_callbacks2);
        return request_decoder2;
      }));
  EXPECT_CALL(request_decoder2, decodeHeaders_(_, false));
  request_encoder2->encodeHeaders(request_headers, false);

  // Add the stream callbacks belatedly. On creation the stream should have
  // been noticed that the connection was backed up. Any new subscriber to
  // stream callbacks should get a callback when they addCallbacks.
  MockStreamCallbacks callbacks2;
  EXPECT_CALL(callbacks2, onAboveWriteBufferHighWatermark());
  request_encoder_->getStream().addCallbacks(callbacks2);

  // Add a third callback to make testing removal mid-watermark call below more interesting.
  MockStreamCallbacks callbacks3;
  EXPECT_CALL(callbacks3, onAboveWriteBufferHighWatermark());
  request_encoder_->getStream().addCallbacks(callbacks3);

  // Now unblock the server's stream. This will cause the bytes to be consumed, flow control
  // updates to be sent, and the client to flush all queued data.
  // For bonus corner case coverage, remove callback2 in the middle of runLowWatermarkCallbacks()
  // and ensure it is not called.
  EXPECT_CALL(callbacks, onBelowWriteBufferLowWatermark()).WillOnce(Invoke([&]() -> void {
    request_encoder_->getStream().removeCallbacks(callbacks2);
  }));
  EXPECT_CALL(callbacks2, onBelowWriteBufferLowWatermark()).Times(0);
  EXPECT_CALL(callbacks3, onBelowWriteBufferLowWatermark());
  server_->getStream(1)->readDisable(false);
  EXPECT_EQ(0, client_->getStream(1)->pending_send_data_.length());
  // The extra 1 byte sent won't trigger another window update, so the final window should be the
  // initial window minus the last 1 byte flush from the client to server.
  EXPECT_EQ(initial_stream_window - 1,
            nghttp2_session_get_stream_local_window_size(server_->session(), 1));
  EXPECT_EQ(initial_stream_window - 1,
            nghttp2_session_get_stream_remote_window_size(client_->session(), 1));
}