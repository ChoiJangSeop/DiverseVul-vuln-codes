TEST(HeaderMapImplTest, LargeCharInHeader) {
  HeaderMapImpl headers;
  LowerCaseString static_key("\x90hello");
  std::string ref_value("value");
  headers.addReference(static_key, ref_value);
  EXPECT_EQ("value", headers.get(static_key)->value().getStringView());
}