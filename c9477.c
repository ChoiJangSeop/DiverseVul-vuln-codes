TEST(Context, RequestFallbackAttributes) {
  NiceMock<StreamInfo::MockStreamInfo> info;
  Http::TestRequestHeaderMapImpl header_map{
      {":method", "POST"},
      {":scheme", "http"},
      {":path", "/meow?yes=1"},
  };
  RequestWrapper request(&header_map, info);

  EXPECT_CALL(info, bytesReceived()).WillRepeatedly(Return(10));

  {
    auto value = request[CelValue::CreateStringView(Size)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    EXPECT_EQ(10, value.value().Int64OrDie());
  }

  {
    auto value = request[CelValue::CreateStringView(UrlPath)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ("/meow", value.value().StringOrDie().value());
  }
}