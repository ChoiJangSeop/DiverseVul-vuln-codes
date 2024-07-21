TEST_P(SslCertficateIntegrationTest, ServerEcdsaClientRsaOnly) {
  server_rsa_cert_ = false;
  server_ecdsa_cert_ = true;
  initialize();
  auto codec_client = makeRawHttpConnection(makeSslClientConnection(rsaOnlyClientOptions()));
  EXPECT_FALSE(codec_client->connected());
  const std::string counter_name = listenerStatPrefix("ssl.connection_error");
  Stats::CounterSharedPtr counter = test_server_->counter(counter_name);
  test_server_->waitForCounterGe(counter_name, 1);
  EXPECT_EQ(1U, counter->value());
  counter->reset();
}