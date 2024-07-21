TEST(HeaderMapImplTest, SetCopy) {
  TestRequestHeaderMapImpl headers;
  LowerCaseString foo("hello");
  headers.setCopy(foo, "world");
  EXPECT_EQ("world", headers.get(foo)->value().getStringView());

  // Overwrite value.
  headers.setCopy(foo, "monde");
  EXPECT_EQ("monde", headers.get(foo)->value().getStringView());

  // Add another foo header.
  headers.addCopy(foo, "monde2");
  EXPECT_EQ(headers.size(), 2);

  // Only the first foo header is overridden.
  headers.setCopy(foo, "override-monde");
  EXPECT_EQ(headers.size(), 2);

  HeaderAndValueCb cb;

  InSequence seq;
  EXPECT_CALL(cb, Call("hello", "override-monde"));
  EXPECT_CALL(cb, Call("hello", "monde2"));
  headers.iterate(cb.asIterateCb());

  // Test setting an empty string and then overriding.
  EXPECT_EQ(2UL, headers.remove(foo));
  EXPECT_EQ(headers.size(), 0);
  const std::string empty;
  headers.setCopy(foo, empty);
  EXPECT_EQ(headers.size(), 1);
  headers.setCopy(foo, "not-empty");
  EXPECT_EQ(headers.get(foo)->value().getStringView(), "not-empty");

  // Use setCopy with inline headers both indirectly and directly.
  headers.clear();
  EXPECT_EQ(headers.size(), 0);
  headers.setCopy(Headers::get().Path, "/");
  EXPECT_EQ(headers.size(), 1);
  EXPECT_EQ(headers.getPathValue(), "/");
  headers.setPath("/foo");
  EXPECT_EQ(headers.size(), 1);
  EXPECT_EQ(headers.getPathValue(), "/foo");
}