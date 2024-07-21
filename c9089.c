TEST_P(QuicHttpIntegrationTest, StopAcceptingConnectionsWhenOverloaded) {
  initialize();
  fake_upstreams_[0]->set_allow_unexpected_disconnects(true);

  // Put envoy in overloaded state and check that it doesn't accept the new client connection.
  updateResource(0.9);
  test_server_->waitForGaugeEq("overload.envoy.overload_actions.stop_accepting_connections.active",
                               1);
  codec_client_ = makeRawHttpConnection(makeClientConnection((lookupPort("http"))));
  EXPECT_TRUE(codec_client_->disconnected());

  // Reduce load a little to allow the connection to be accepted connection.
  updateResource(0.8);
  test_server_->waitForGaugeEq("overload.envoy.overload_actions.stop_accepting_connections.active",
                               0);
  codec_client_ = makeHttpConnection(makeClientConnection((lookupPort("http"))));
  auto response = codec_client_->makeHeaderOnlyRequest(default_request_headers_);
  waitForNextUpstreamRequest(0);
  // Send response headers, but hold response body for now.
  upstream_request_->encodeHeaders(default_response_headers_, /*end_stream=*/false);

  updateResource(0.95);
  test_server_->waitForGaugeEq("overload.envoy.overload_actions.stop_accepting_requests.active", 1);
  // Existing request should be able to finish.
  upstream_request_->encodeData(10, true);
  response->waitForEndStream();
  EXPECT_TRUE(response->complete());
  EXPECT_EQ("200", response->headers().getStatusValue());

  // New request should be rejected.
  auto response2 = codec_client_->makeHeaderOnlyRequest(default_request_headers_);
  response2->waitForEndStream();
  EXPECT_EQ("503", response2->headers().getStatusValue());
  EXPECT_EQ("envoy overloaded", response2->body());
  codec_client_->close();

  EXPECT_TRUE(makeRawHttpConnection(makeClientConnection((lookupPort("http"))))->disconnected());
}