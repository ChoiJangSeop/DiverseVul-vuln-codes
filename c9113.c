HttpIntegrationTest::makeHttpConnection(Network::ClientConnectionPtr&& conn) {
  auto codec = makeRawHttpConnection(std::move(conn));
  EXPECT_TRUE(codec->connected()) << codec->connection()->transportFailureReason();
  return codec;
}