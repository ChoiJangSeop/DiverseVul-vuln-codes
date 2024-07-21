IntegrationStreamDecoderPtr HttpIntegrationTest::sendRequestAndWaitForResponse(
    const Http::TestHeaderMapImpl& request_headers, uint32_t request_body_size,
    const Http::TestHeaderMapImpl& response_headers, uint32_t response_size, int upstream_index) {
  ASSERT(codec_client_ != nullptr);
  // Send the request to Envoy.
  IntegrationStreamDecoderPtr response;
  if (request_body_size) {
    response = codec_client_->makeRequestWithBody(request_headers, request_body_size);
  } else {
    response = codec_client_->makeHeaderOnlyRequest(request_headers);
  }
  waitForNextUpstreamRequest(upstream_index);
  // Send response headers, and end_stream if there is no response body.
  upstream_request_->encodeHeaders(response_headers, response_size == 0);
  // Send any response data, with end_stream true.
  if (response_size) {
    upstream_request_->encodeData(response_size, true);
  }
  // Wait for the response to be read by the codec client.
  response->waitForEndStream();
  return response;
}