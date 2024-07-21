TEST(HeaderMapImplTest, SetRemovesAllValues) {
  HeaderMapImpl headers;

  LowerCaseString key1("hello");
  LowerCaseString key2("olleh");
  std::string ref_value1("world");
  std::string ref_value2("planet");
  std::string ref_value3("globe");
  std::string ref_value4("earth");
  std::string ref_value5("blue marble");

  headers.addReference(key1, ref_value1);
  headers.addReference(key2, ref_value2);
  headers.addReference(key1, ref_value3);
  headers.addReference(key1, ref_value4);

  using MockCb = testing::MockFunction<void(const std::string&, const std::string&)>;

  {
    MockCb cb;

    InSequence seq;
    EXPECT_CALL(cb, Call("hello", "world"));
    EXPECT_CALL(cb, Call("olleh", "planet"));
    EXPECT_CALL(cb, Call("hello", "globe"));
    EXPECT_CALL(cb, Call("hello", "earth"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);
  }

  headers.setReference(key1, ref_value5); // set moves key to end

  {
    MockCb cb;

    InSequence seq;
    EXPECT_CALL(cb, Call("olleh", "planet"));
    EXPECT_CALL(cb, Call("hello", "blue marble"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);
  }
}