TEST(HeaderMapImplTest, MoveIntoInline) {
  HeaderMapImpl headers;
  HeaderString key;
  key.setCopy(Headers::get().CacheControl.get());
  HeaderString value;
  value.setCopy("hello", 5);
  headers.addViaMove(std::move(key), std::move(value));
  EXPECT_EQ("cache-control", headers.CacheControl()->key().getStringView());
  EXPECT_EQ("hello", headers.CacheControl()->value().getStringView());

  HeaderString key2;
  key2.setCopy(Headers::get().CacheControl.get().c_str(), Headers::get().CacheControl.get().size());
  HeaderString value2;
  value2.setCopy("there", 5);
  headers.addViaMove(std::move(key2), std::move(value2));
  EXPECT_EQ("cache-control", headers.CacheControl()->key().getStringView());
  EXPECT_EQ("hello,there", headers.CacheControl()->value().getStringView());
}