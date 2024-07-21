TEST_F(RouterTest, RetryUpstreamResetResponseStarted) {
  NiceMock<Http::MockRequestEncoder> encoder1;
  Http::ResponseDecoder* response_decoder = nullptr;
  EXPECT_CALL(cm_.thread_local_cluster_.conn_pool_, newStream(_, _))
      .WillOnce(Invoke(
          [&](Http::ResponseDecoder& decoder,
              Http::ConnectionPool::Callbacks& callbacks) -> Http::ConnectionPool::Cancellable* {
            response_decoder = &decoder;
            callbacks.onPoolReady(encoder1, cm_.thread_local_cluster_.conn_pool_.host_,
                                  upstream_stream_info_, Http::Protocol::Http10);
            return nullptr;
          }));
  expectResponseTimerCreate();

  Http::TestRequestHeaderMapImpl headers{{"x-envoy-retry-on", "5xx"}, {"x-envoy-internal", "true"}};
  HttpTestUtility::addDefaultHeaders(headers);
  router_.decodeHeaders(headers, true);
  EXPECT_EQ(1U,
            callbacks_.route_->route_entry_.virtual_cluster_.stats().upstream_rq_total_.value());

  // Since the response is already started we don't retry.
  EXPECT_CALL(*router_.retry_state_, shouldRetryHeaders(_, _)).WillOnce(Return(RetryStatus::No));
  EXPECT_CALL(callbacks_, encodeHeaders_(_, false));
  Http::ResponseHeaderMapPtr response_headers(
      new Http::TestResponseHeaderMapImpl{{":status", "200"}});
  EXPECT_CALL(cm_.thread_local_cluster_.conn_pool_.host_->outlier_detector_,
              putHttpResponseCode(200));
  response_decoder->decodeHeaders(std::move(response_headers), false);
  EXPECT_CALL(cm_.thread_local_cluster_.conn_pool_.host_->outlier_detector_,
              putResult(Upstream::Outlier::Result::LocalOriginConnectFailed, _));
  // Normally, sendLocalReply will actually send the reply, but in this case the
  // HCM will detect the headers have already been sent and not route through
  // the encoder again.
  EXPECT_CALL(callbacks_, sendLocalReply(_, _, _, _, _)).WillOnce(testing::InvokeWithoutArgs([] {
  }));
  encoder1.stream_.resetStream(Http::StreamResetReason::RemoteReset);
  // For normal HTTP, once we have a 200 we consider this a success, even if a
  // later reset occurs.
  EXPECT_TRUE(verifyHostUpstreamStats(1, 0));
  EXPECT_EQ(1U,
            callbacks_.route_->route_entry_.virtual_cluster_.stats().upstream_rq_total_.value());
}