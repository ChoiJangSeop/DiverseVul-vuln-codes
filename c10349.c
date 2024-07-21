TEST_F(ConnectionManagerUtilityTest, SanitizeEmptyPath) {
  ON_CALL(config_, shouldNormalizePath()).WillByDefault(Return(false));
  TestRequestHeaderMapImpl original_headers;

  TestRequestHeaderMapImpl header_map(original_headers);
  EXPECT_TRUE(ConnectionManagerUtility::maybeNormalizePath(header_map, config_));
  EXPECT_EQ(original_headers, header_map);
}