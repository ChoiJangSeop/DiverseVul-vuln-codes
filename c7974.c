TEST(HeaderMapImplTest, SetReferenceKey) {
  HeaderMapImpl headers;
  LowerCaseString foo("hello");
  headers.setReferenceKey(foo, "world");
  EXPECT_NE("world", headers.get(foo)->value().getStringView().data());
  EXPECT_EQ("world", headers.get(foo)->value().getStringView());

  headers.setReferenceKey(foo, "monde");
  EXPECT_NE("monde", headers.get(foo)->value().getStringView().data());
  EXPECT_EQ("monde", headers.get(foo)->value().getStringView());
}