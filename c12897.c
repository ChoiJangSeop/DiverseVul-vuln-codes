TEST_F(AllowMissingInAndOfOrListTest, GoodAndBadJwts) {
  EXPECT_CALL(mock_cb_, onComplete(Status::Ok));
  // Use the token with example.com issuer for x-other.
  auto headers =
      Http::TestRequestHeaderMapImpl{{kExampleHeader, GoodToken}, {kOtherHeader, GoodToken}};
  context_ = Verifier::createContext(headers, parent_span_, &mock_cb_);
  verifier_->verify(context_);
  EXPECT_THAT(headers, JwtOutputSuccess(kExampleHeader));
  EXPECT_THAT(headers, JwtOutputFailedOrIgnore(kOtherHeader));
}