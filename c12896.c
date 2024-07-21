TEST_F(SingleAllowMissingInOrListTest, MissingIssToken) {
  EXPECT_CALL(mock_cb_, onComplete(Status::Ok));
  auto headers = Http::TestRequestHeaderMapImpl{{kExampleHeader, ES256WithoutIssToken}};
  context_ = Verifier::createContext(headers, parent_span_, &mock_cb_);
  verifier_->verify(context_);
  EXPECT_THAT(headers, JwtOutputFailedOrIgnore(kExampleHeader));
}