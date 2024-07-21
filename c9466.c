TEST(MatchHeadersTest, MayMatchOneOrMoreRequestHeader) {
  TestRequestHeaderMapImpl headers{{"some-header", "a"}, {"other-header", "b"}};

  const std::string yaml = R"EOF(
name: match-header
regex_match: (a|b)
  )EOF";

  std::vector<HeaderUtility::HeaderDataPtr> header_data;
  header_data.push_back(
      std::make_unique<HeaderUtility::HeaderData>(parseHeaderMatcherFromYaml(yaml)));
  EXPECT_FALSE(HeaderUtility::matchHeaders(headers, header_data));

  headers.addCopy("match-header", "a");
  EXPECT_TRUE(HeaderUtility::matchHeaders(headers, header_data));
  headers.addCopy("match-header", "b");
  EXPECT_TRUE(HeaderUtility::matchHeaders(headers, header_data));
}