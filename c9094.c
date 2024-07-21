HttpIntegrationTest::makeRawHttpConnection(Network::ClientConnectionPtr&& conn) {
  std::shared_ptr<Upstream::MockClusterInfo> cluster{new NiceMock<Upstream::MockClusterInfo>()};
  cluster->max_response_headers_count_ = 200;
  cluster->http2_options_.set_allow_connect(true);
  cluster->http2_options_.set_allow_metadata(true);
  cluster->http1_settings_.enable_trailers_ = true;
  Upstream::HostDescriptionConstSharedPtr host_description{Upstream::makeTestHostDescription(
      cluster, fmt::format("tcp://{}:80", Network::Test::getLoopbackAddressUrlString(version_)))};
  return std::make_unique<IntegrationCodecClient>(*dispatcher_, std::move(conn), host_description,
                                                  downstream_protocol_);
}