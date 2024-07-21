TEST_P(AltsIntegrationTestClientInvalidPeer, ClientValidationFail) {
  initialize();
  codec_client_ = makeRawHttpConnection(makeAltsConnection());
  EXPECT_FALSE(codec_client_->connected());
}