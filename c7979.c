void HttpIntegrationTest::waitForNextUpstreamRequest(uint64_t upstream_index) {
  waitForNextUpstreamRequest(std::vector<uint64_t>({upstream_index}));
}