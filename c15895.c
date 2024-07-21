TEST_F(OAuth2Test, OAuthBearerTokenFlowFromQueryParameters) {
  Http::TestRequestHeaderMapImpl request_headers_before{
      {Http::Headers::get().Path.get(), "/test?role=bearer&token=xyz-queryparam-token"},
      {Http::Headers::get().Host.get(), "traffic.example.com"},
      {Http::Headers::get().Method.get(), Http::Headers::get().MethodValues.Get},
      {Http::Headers::get().Scheme.get(), "https"},
  };
  Http::TestRequestHeaderMapImpl request_headers_after{
      {Http::Headers::get().Path.get(), "/test?role=bearer&token=xyz-queryparam-token"},
      {Http::Headers::get().Host.get(), "traffic.example.com"},
      {Http::Headers::get().Method.get(), Http::Headers::get().MethodValues.Get},
      {Http::Headers::get().Scheme.get(), "https"},
      {Http::CustomHeaders::get().Authorization.get(), "Bearer xyz-queryparam-token"},
  };

  // Fail the validation to trigger the OAuth flow.
  EXPECT_CALL(*validator_, setParams(_, _));
  EXPECT_CALL(*validator_, isValid()).WillOnce(Return(false));

  EXPECT_EQ(Http::FilterHeadersStatus::Continue,
            filter_->decodeHeaders(request_headers_before, false));

  // Expected decoded headers after the callback & validation of the bearer token is complete.
  EXPECT_EQ(request_headers_before, request_headers_after);
}