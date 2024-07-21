TEST_P(SdsDynamicDownstreamIntegrationTest, WrongSecretFirst) {
  on_server_init_function_ = [this]() {
    createSdsStream(*(fake_upstreams_[1]));
    sendSdsResponse(getWrongSecret(server_cert_));
  };
  initialize();

  codec_client_ = makeRawHttpConnection(makeSslClientConnection());
  // the connection state is not connected.
  EXPECT_FALSE(codec_client_->connected());
  codec_client_->connection()->close(Network::ConnectionCloseType::NoFlush);

  sendSdsResponse(getServerSecret());

  // Wait for ssl_context_updated_by_sds counter.
  test_server_->waitForCounterGe(
      listenerStatPrefix("server_ssl_socket_factory.ssl_context_update_by_sds"), 1);

  ConnectionCreationFunction creator = [&]() -> Network::ClientConnectionPtr {
    return makeSslClientConnection();
  };
  testRouterHeaderOnlyRequestAndResponse(&creator);
}