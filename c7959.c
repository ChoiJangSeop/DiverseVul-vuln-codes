TEST(HeaderMapImplTest, DoubleInlineAdd) {
  {
    HeaderMapImpl headers;
    const std::string foo("foo");
    const std::string bar("bar");
    headers.addReference(Headers::get().ContentLength, foo);
    headers.addReference(Headers::get().ContentLength, bar);
    EXPECT_EQ("foo,bar", headers.ContentLength()->value().getStringView());
    EXPECT_EQ(1UL, headers.size());
  }
  {
    HeaderMapImpl headers;
    headers.addReferenceKey(Headers::get().ContentLength, "foo");
    headers.addReferenceKey(Headers::get().ContentLength, "bar");
    EXPECT_EQ("foo,bar", headers.ContentLength()->value().getStringView());
    EXPECT_EQ(1UL, headers.size());
  }
  {
    HeaderMapImpl headers;
    headers.addReferenceKey(Headers::get().ContentLength, 5);
    headers.addReferenceKey(Headers::get().ContentLength, 6);
    EXPECT_EQ("5,6", headers.ContentLength()->value().getStringView());
    EXPECT_EQ(1UL, headers.size());
  }
  {
    HeaderMapImpl headers;
    const std::string foo("foo");
    headers.addReference(Headers::get().ContentLength, foo);
    headers.addReferenceKey(Headers::get().ContentLength, 6);
    EXPECT_EQ("foo,6", headers.ContentLength()->value().getStringView());
    EXPECT_EQ(1UL, headers.size());
  }
}