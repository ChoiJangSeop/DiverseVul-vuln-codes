TEST_F(OAuth2Test, OAuthTestFullFlowPostWithParameters) {
  // First construct the initial request to the oauth filter with URI parameters.
  Http::TestRequestHeaderMapImpl first_request_headers{
      {Http::Headers::get().Path.get(), "/test?name=admin&level=trace"},
      {Http::Headers::get().Host.get(), "traffic.example.com"},
      {Http::Headers::get().Method.get(), Http::Headers::get().MethodValues.Post},
      {Http::Headers::get().Scheme.get(), "https"},
  };

  // This is the immediate response - a redirect to the auth cluster.
  Http::TestResponseHeaderMapImpl first_response_headers{
      {Http::Headers::get().Status.get(), "302"},
      {Http::Headers::get().Location.get(),
       "https://auth.example.com/oauth/"
       "authorize/?client_id=" +
           TEST_CLIENT_ID + "&scope=" + TEST_ENCODED_AUTH_SCOPES +
           "&response_type=code&"
           "redirect_uri=https%3A%2F%2Ftraffic.example.com%2F"
           "_oauth&state=https%3A%2F%2Ftraffic.example.com%2Ftest%"
           "3Fname%3Dadmin%26level%3Dtrace"
           "&resource=oauth2-resource&resource=http%3A%2F%2Fexample.com"
           "&resource=https%3A%2F%2Fexample.com"},
  };

  // Fail the validation to trigger the OAuth flow.
  EXPECT_CALL(*validator_, setParams(_, _));
  EXPECT_CALL(*validator_, isValid()).WillOnce(Return(false));

  // Check that the redirect includes the escaped parameter characters, '?', '&' and '='.
  EXPECT_CALL(decoder_callbacks_, encodeHeaders_(HeaderMapEqualRef(&first_response_headers), true));

  // This represents the beginning of the OAuth filter.
  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter_->decodeHeaders(first_request_headers, false));

  // This represents the callback request from the authorization server.
  Http::TestRequestHeaderMapImpl second_request_headers{
      {Http::Headers::get().Path.get(), "/_oauth?code=123&state=https%3A%2F%2Ftraffic.example.com%"
                                        "2Ftest%3Fname%3Dadmin%26level%3Dtrace"},
      {Http::Headers::get().Host.get(), "traffic.example.com"},
      {Http::Headers::get().Method.get(), Http::Headers::get().MethodValues.Get},
      {Http::Headers::get().Scheme.get(), "https"},
  };

  // Deliberately fail the HMAC validation check.
  EXPECT_CALL(*validator_, setParams(_, _));
  EXPECT_CALL(*validator_, isValid()).WillOnce(Return(false));

  EXPECT_CALL(*oauth_client_, asyncGetAccessToken("123", TEST_CLIENT_ID, "asdf_client_secret_fdsa",
                                                  "https://traffic.example.com" + TEST_CALLBACK));

  // Invoke the callback logic. As a side effect, state_ will be populated.
  EXPECT_EQ(Http::FilterHeadersStatus::StopAllIterationAndBuffer,
            filter_->decodeHeaders(second_request_headers, false));

  EXPECT_EQ(1, config_->stats().oauth_unauthorized_rq_.value());
  EXPECT_EQ(config_->clusterName(), "auth.example.com");

  // Expected response after the callback & validation is complete - verifying we kept the
  // state and method of the original request, including the query string parameters.
  Http::TestRequestHeaderMapImpl second_response_headers{
      {Http::Headers::get().Status.get(), "302"},
      {Http::Headers::get().SetCookie.get(),
       "OauthHMAC="
       "NWUzNzE5MWQwYTg0ZjA2NjIyMjVjMzk3MzY3MzMyZmE0NjZmMWI2MjI1NWFhNDhkYjQ4NDFlZmRiMTVmMTk0MQ==;"
       "version=1;path=/;Max-Age=;secure;HttpOnly"},
      {Http::Headers::get().SetCookie.get(),
       "OauthExpires=;version=1;path=/;Max-Age=;secure;HttpOnly"},
      {Http::Headers::get().SetCookie.get(), "BearerToken=;version=1;path=/;Max-Age=;secure"},
      {Http::Headers::get().Location.get(),
       "https://traffic.example.com/test?name=admin&level=trace"},
  };

  EXPECT_CALL(decoder_callbacks_,
              encodeHeaders_(HeaderMapEqualRef(&second_response_headers), true));
  EXPECT_CALL(decoder_callbacks_, continueDecoding());

  filter_->finishFlow();
}