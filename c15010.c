TEST_F(RouterTest, MissingRequiredHeaders) {
  NiceMock<Http::MockRequestEncoder> encoder;
  Http::ResponseDecoder* response_decoder = nullptr;
  EXPECT_CALL(cm_.thread_local_cluster_.conn_pool_, newStream(_, _))
      .WillOnce(Invoke(
          [&](Http::ResponseDecoder& decoder,
              Http::ConnectionPool::Callbacks& callbacks) -> Http::ConnectionPool::Cancellable* {
            response_decoder = &decoder;
            callbacks.onPoolReady(encoder, cm_.thread_local_cluster_.conn_pool_.host_,
                                  upstream_stream_info_, Http::Protocol::Http10);
            return nullptr;
          }));
  expectResponseTimerCreate();

  Http::TestRequestHeaderMapImpl headers;
  HttpTestUtility::addDefaultHeaders(headers);
  headers.removeMethod();

  EXPECT_CALL(encoder, encodeHeaders(_, _))
      .WillOnce(Invoke([](const Http::RequestHeaderMap& headers, bool) -> Http::Status {
        return Http::HeaderUtility::checkRequiredRequestHeaders(headers);
      }));
  EXPECT_CALL(
      callbacks_,
      sendLocalReply(Http::Code::ServiceUnavailable,
                     testing::Eq("missing required header: :method"), _, _,
                     "filter_removed_required_request_headers{missing required header: :method}"))
      .WillOnce(testing::InvokeWithoutArgs([] {}));
  router_.decodeHeaders(headers, true);
  router_.onDestroy();
}