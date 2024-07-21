TEST(Context, InvalidRequest) {
  Http::TestRequestHeaderMapImpl header_map{{"referer", "dogs.com"}};
  HeadersWrapper<Http::RequestHeaderMap> headers(&header_map);
  auto header = headers[CelValue::CreateStringView("dogs.com\n")];
  EXPECT_FALSE(header.has_value());
}