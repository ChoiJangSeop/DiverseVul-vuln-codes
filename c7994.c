TEST(HeaderMapImplTest, DoubleCookieAdd) {
  HeaderMapImpl headers;
  const std::string foo("foo");
  const std::string bar("bar");
  const LowerCaseString& set_cookie = Http::Headers::get().SetCookie;
  headers.addReference(set_cookie, foo);
  headers.addReference(set_cookie, bar);
  EXPECT_EQ(2UL, headers.size());

  std::vector<absl::string_view> out;
  Http::HeaderUtility::getAllOfHeader(headers, "set-cookie", out);
  ASSERT_EQ(out.size(), 2);
  ASSERT_EQ(out[0], "foo");
  ASSERT_EQ(out[1], "bar");
}