TEST(HeaderMapImplTest, DoubleInlineSet) {
  HeaderMapImpl headers;
  headers.setReferenceKey(Headers::get().ContentType, "blah");
  headers.setReferenceKey(Headers::get().ContentType, "text/html");
  EXPECT_EQ("text/html", headers.ContentType()->value().getStringView());
  EXPECT_EQ(1UL, headers.size());
}