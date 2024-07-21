TEST_P(Http2CodecImplTest, PingFlood) {
  initialize();

  TestRequestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(_, false));
  request_encoder_->encodeHeaders(request_headers, false);

  // Send one frame above the outbound control queue size limit
  for (uint32_t i = 0; i < CommonUtility::OptionsLimits::DEFAULT_MAX_OUTBOUND_CONTROL_FRAMES + 1;
       ++i) {
    EXPECT_EQ(0, nghttp2_submit_ping(client_->session(), NGHTTP2_FLAG_NONE, nullptr));
  }

  int ack_count = 0;
  Buffer::OwnedImpl buffer;
  ON_CALL(server_connection_, write(_, _))
      .WillByDefault(Invoke([&buffer, &ack_count](Buffer::Instance& frame, bool) {
        ++ack_count;
        buffer.move(frame);
      }));

  EXPECT_THROW(client_->sendPendingFrames(), ServerCodecError);
  EXPECT_EQ(ack_count, CommonUtility::OptionsLimits::DEFAULT_MAX_OUTBOUND_CONTROL_FRAMES);
  EXPECT_EQ(1, stats_store_.counter("http2.outbound_control_flood").value());
}