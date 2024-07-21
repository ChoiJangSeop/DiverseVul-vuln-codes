TEST(Context, RequestAttributes) {
  NiceMock<StreamInfo::MockStreamInfo> info;
  NiceMock<StreamInfo::MockStreamInfo> empty_info;
  Http::TestRequestHeaderMapImpl header_map{
      {":method", "POST"},           {":scheme", "http"},      {":path", "/meow?yes=1"},
      {":authority", "kittens.com"}, {"referer", "dogs.com"},  {"user-agent", "envoy-mobile"},
      {"content-length", "10"},      {"x-request-id", "blah"},
  };
  RequestWrapper request(&header_map, info);
  RequestWrapper empty_request(nullptr, empty_info);

  EXPECT_CALL(info, bytesReceived()).WillRepeatedly(Return(10));
  // "2018-04-03T23:06:09.123Z".
  const SystemTime start_time(std::chrono::milliseconds(1522796769123));
  EXPECT_CALL(info, startTime()).WillRepeatedly(Return(start_time));
  absl::optional<std::chrono::nanoseconds> dur = std::chrono::nanoseconds(15000000);
  EXPECT_CALL(info, requestComplete()).WillRepeatedly(Return(dur));
  EXPECT_CALL(info, protocol()).WillRepeatedly(Return(Http::Protocol::Http2));

  // stub methods
  EXPECT_EQ(0, request.size());
  EXPECT_FALSE(request.empty());

  {
    auto value = request[CelValue::CreateStringView(Undefined)];
    EXPECT_FALSE(value.has_value());
  }

  {
    auto value = request[CelValue::CreateInt64(13)];
    EXPECT_FALSE(value.has_value());
  }

  {
    auto value = request[CelValue::CreateStringView(Scheme)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ("http", value.value().StringOrDie().value());
  }

  {
    auto value = empty_request[CelValue::CreateStringView(Scheme)];
    EXPECT_FALSE(value.has_value());
  }

  {
    auto value = request[CelValue::CreateStringView(Host)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ("kittens.com", value.value().StringOrDie().value());
  }

  {
    auto value = request[CelValue::CreateStringView(Path)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ("/meow?yes=1", value.value().StringOrDie().value());
  }

  {
    auto value = request[CelValue::CreateStringView(UrlPath)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ("/meow", value.value().StringOrDie().value());
  }

  {
    auto value = request[CelValue::CreateStringView(Method)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ("POST", value.value().StringOrDie().value());
  }

  {
    auto value = request[CelValue::CreateStringView(Referer)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ("dogs.com", value.value().StringOrDie().value());
  }

  {
    auto value = request[CelValue::CreateStringView(UserAgent)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ("envoy-mobile", value.value().StringOrDie().value());
  }

  {
    auto value = request[CelValue::CreateStringView(ID)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ("blah", value.value().StringOrDie().value());
  }

  {
    auto value = request[CelValue::CreateStringView(Size)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    EXPECT_EQ(10, value.value().Int64OrDie());
  }

  {
    auto value = request[CelValue::CreateStringView(TotalSize)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    // this includes the headers size
    EXPECT_EQ(138, value.value().Int64OrDie());
  }

  {
    auto value = empty_request[CelValue::CreateStringView(TotalSize)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    // this includes the headers size
    EXPECT_EQ(0, value.value().Int64OrDie());
  }

  {
    auto value = request[CelValue::CreateStringView(Time)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsTimestamp());
    EXPECT_EQ("2018-04-03T23:06:09.123+00:00", absl::FormatTime(value.value().TimestampOrDie()));
  }

  {
    auto value = request[CelValue::CreateStringView(Headers)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsMap());
    auto& map = *value.value().MapOrDie();
    EXPECT_FALSE(map.empty());
    EXPECT_EQ(8, map.size());

    auto header = map[CelValue::CreateStringView(Referer)];
    EXPECT_TRUE(header.has_value());
    ASSERT_TRUE(header.value().IsString());
    EXPECT_EQ("dogs.com", header.value().StringOrDie().value());
  }

  {
    auto value = request[CelValue::CreateStringView(Duration)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsDuration());
    EXPECT_EQ("15ms", absl::FormatDuration(value.value().DurationOrDie()));
  }

  {
    auto value = empty_request[CelValue::CreateStringView(Duration)];
    EXPECT_FALSE(value.has_value());
  }

  {
    auto value = request[CelValue::CreateStringView(Protocol)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ("HTTP/2", value.value().StringOrDie().value());
  }

  {
    auto value = empty_request[CelValue::CreateStringView(Protocol)];
    EXPECT_FALSE(value.has_value());
  }
}