TEST_F(GroupVerifierTest, TestRequiresAnyWithAllowMissingButOk) {
  TestUtility::loadFromYaml(RequiresAnyConfig, proto_config_);
  proto_config_.mutable_rules(0)
      ->mutable_requires()
      ->mutable_requires_any()
      ->add_requirements()
      ->mutable_allow_missing();

  createAsyncMockAuthsAndVerifier(std::vector<std::string>{"example_provider", "other_provider"});
  EXPECT_CALL(mock_cb_, onComplete(Status::Ok));

  auto headers = Http::TestRequestHeaderMapImpl{};
  context_ = Verifier::createContext(headers, parent_span_, &mock_cb_);
  verifier_->verify(context_);
  callbacks_["example_provider"](Status::JwtMissed);
  callbacks_["other_provider"](Status::JwtUnknownIssuer);
}