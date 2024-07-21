void Http2UpstreamIntegrationTest::manySimultaneousRequests(uint32_t request_bytes, uint32_t) {
  TestRandomGenerator rand;
  const uint32_t num_requests = 50;
  std::vector<Http::RequestEncoder*> encoders;
  std::vector<IntegrationStreamDecoderPtr> responses;
  std::vector<int> response_bytes;
  autonomous_upstream_ = true;
  initialize();

  codec_client_ = makeHttpConnection(lookupPort("http"));
  for (uint32_t i = 0; i < num_requests; ++i) {
    response_bytes.push_back(rand.random() % (1024 * 2));
    auto headers = Http::TestRequestHeaderMapImpl{
        {":method", "POST"},
        {":path", "/test/long/url"},
        {":scheme", "http"},
        {":authority", "host"},
        {AutonomousStream::RESPONSE_SIZE_BYTES, std::to_string(response_bytes[i])},
        {AutonomousStream::EXPECT_REQUEST_SIZE_BYTES, std::to_string(request_bytes)}};
    if (i % 2 == 0) {
      headers.addCopy(AutonomousStream::RESET_AFTER_REQUEST, "yes");
    }
    auto encoder_decoder = codec_client_->startRequest(headers);
    encoders.push_back(&encoder_decoder.first);
    responses.push_back(std::move(encoder_decoder.second));
    codec_client_->sendData(*encoders[i], request_bytes, true);
  }

  for (uint32_t i = 0; i < num_requests; ++i) {
    responses[i]->waitForEndStream();
    if (i % 2 != 0) {
      EXPECT_TRUE(responses[i]->complete());
      EXPECT_EQ("200", responses[i]->headers().getStatusValue());
      EXPECT_EQ(response_bytes[i], responses[i]->body().length());
    } else {
      // Upstream stream reset.
      EXPECT_EQ("503", responses[i]->headers().getStatusValue());
    }
  }
}