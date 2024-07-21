TEST_F(OAuth2Test, OAuthBearerTokenFlowFromHeader) {
  Http::TestRequestHeaderMapImpl request_headers_before{
      {Http::Headers::get().Path.get(), "/test?role=bearer"},
      {Http::Headers::get().Host.get(), "traffic.example.com"},
      {Http::Headers::get().Method.get(), Http::Headers::get().MethodValues.Get},
      {Http::Headers::get().Scheme.get(), "https"},
      {Http::CustomHeaders::get().Authorization.get(), "Bearer xyz-header-token"},
  };
  // Expected decoded headers after the callback & validation of the bearer token is complete.
  Http::TestRequestHeaderMapImpl request_headers_after{
      {Http::Headers::get().Path.get(), "/test?role=bearer"},
      {Http::Headers::get().Host.get(), "traffic.example.com"},
      {Http::Headers::get().Method.get(), Http::Headers::get().MethodValues.Get},
      {Http::Headers::get().Scheme.get(), "https"},
      {Http::CustomHeaders::get().Authorization.get(), "Bearer xyz-header-token"},
  };

  // Fail the validation to trigger the OAuth flow.
  EXPECT_CALL(*validator_, setParams(_, _));
  EXPECT_CALL(*validator_, isValid()).WillOnce(Return(false));

  EXPECT_EQ(Http::FilterHeadersStatus::Continue,
            filter_->decodeHeaders(request_headers_before, false));

  // Finally, expect that the header map had OAuth information appended to it.
  EXPECT_EQ(request_headers_before, request_headers_after);
}