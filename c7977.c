TEST(HeaderMapImplTest, RemoveRegex) {
  // These will match.
  LowerCaseString key1 = LowerCaseString("X-prefix-foo");
  LowerCaseString key3 = LowerCaseString("X-Prefix-");
  LowerCaseString key5 = LowerCaseString("x-prefix-eep");
  // These will not.
  LowerCaseString key2 = LowerCaseString(" x-prefix-foo");
  LowerCaseString key4 = LowerCaseString("y-x-prefix-foo");

  HeaderMapImpl headers;
  headers.addReference(key1, "value");
  headers.addReference(key2, "value");
  headers.addReference(key3, "value");
  headers.addReference(key4, "value");
  headers.addReference(key5, "value");

  // Test removing the first header, middle headers, and the end header.
  headers.removePrefix(LowerCaseString("x-prefix-"));
  EXPECT_EQ(nullptr, headers.get(key1));
  EXPECT_NE(nullptr, headers.get(key2));
  EXPECT_EQ(nullptr, headers.get(key3));
  EXPECT_NE(nullptr, headers.get(key4));
  EXPECT_EQ(nullptr, headers.get(key5));

  // Remove all headers.
  headers.removePrefix(LowerCaseString(""));
  EXPECT_EQ(nullptr, headers.get(key2));
  EXPECT_EQ(nullptr, headers.get(key4));

  // Add inline and remove by regex
  headers.insertContentLength().value(5);
  EXPECT_EQ("5", headers.ContentLength()->value().getStringView());
  EXPECT_EQ(1UL, headers.size());
  EXPECT_FALSE(headers.empty());
  headers.removePrefix(LowerCaseString("content"));
  EXPECT_EQ(nullptr, headers.ContentLength());
}