TEST(Context, ResponseAttributes) {
  NiceMock<StreamInfo::MockStreamInfo> info;
  NiceMock<StreamInfo::MockStreamInfo> empty_info;
  const std::string header_name = "test-header";
  const std::string trailer_name = "test-trailer";
  const std::string grpc_status = "grpc-status";
  Http::TestResponseHeaderMapImpl header_map{{header_name, "a"}};
  Http::TestResponseTrailerMapImpl trailer_map{{trailer_name, "b"}, {grpc_status, "8"}};
  ResponseWrapper response(&header_map, &trailer_map, info);
  ResponseWrapper empty_response(nullptr, nullptr, empty_info);

  EXPECT_CALL(info, responseCode()).WillRepeatedly(Return(404));
  EXPECT_CALL(info, bytesSent()).WillRepeatedly(Return(123));
  EXPECT_CALL(info, responseFlags()).WillRepeatedly(Return(0x1));

  const absl::optional<std::string> code_details = "unauthorized";
  EXPECT_CALL(info, responseCodeDetails()).WillRepeatedly(ReturnRef(code_details));

  {
    auto value = response[CelValue::CreateStringView(Undefined)];
    EXPECT_FALSE(value.has_value());
  }

  {
    auto value = response[CelValue::CreateInt64(13)];
    EXPECT_FALSE(value.has_value());
  }

  {
    auto value = response[CelValue::CreateStringView(Size)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    EXPECT_EQ(123, value.value().Int64OrDie());
  }

  {
    auto value = response[CelValue::CreateStringView(TotalSize)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    EXPECT_EQ(160, value.value().Int64OrDie());
  }

  {
    auto value = empty_response[CelValue::CreateStringView(TotalSize)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    EXPECT_EQ(0, value.value().Int64OrDie());
  }

  {
    auto value = response[CelValue::CreateStringView(Code)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    EXPECT_EQ(404, value.value().Int64OrDie());
  }

  {
    auto value = response[CelValue::CreateStringView(CodeDetails)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsString());
    EXPECT_EQ(code_details.value(), value.value().StringOrDie().value());
  }

  {
    auto value = response[CelValue::CreateStringView(Headers)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsMap());
    auto& map = *value.value().MapOrDie();
    EXPECT_FALSE(map.empty());
    EXPECT_EQ(1, map.size());

    auto header = map[CelValue::CreateStringView(header_name)];
    EXPECT_TRUE(header.has_value());
    ASSERT_TRUE(header.value().IsString());
    EXPECT_EQ("a", header.value().StringOrDie().value());

    auto missing = map[CelValue::CreateStringView(Undefined)];
    EXPECT_FALSE(missing.has_value());
  }

  {
    auto value = response[CelValue::CreateStringView(Trailers)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsMap());
    auto& map = *value.value().MapOrDie();
    EXPECT_FALSE(map.empty());
    EXPECT_EQ(2, map.size());

    auto header = map[CelValue::CreateString(&trailer_name)];
    EXPECT_TRUE(header.has_value());
    ASSERT_TRUE(header.value().IsString());
    EXPECT_EQ("b", header.value().StringOrDie().value());
  }

  {
    auto value = response[CelValue::CreateStringView(Flags)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    EXPECT_EQ(0x1, value.value().Int64OrDie());
  }

  {
    auto value = response[CelValue::CreateStringView(GrpcStatus)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    EXPECT_EQ(0x8, value.value().Int64OrDie());
  }

  {
    auto value = empty_response[CelValue::CreateStringView(GrpcStatus)];
    EXPECT_FALSE(value.has_value());
  }

  {
    auto value = empty_response[CelValue::CreateStringView(Code)];
    EXPECT_FALSE(value.has_value());
  }

  {
    auto value = empty_response[CelValue::CreateStringView(CodeDetails)];
    EXPECT_FALSE(value.has_value());
  }

  {
    Http::TestResponseHeaderMapImpl header_map{{header_name, "a"}, {grpc_status, "7"}};
    Http::TestResponseTrailerMapImpl trailer_map{{trailer_name, "b"}};
    ResponseWrapper response_header_status(&header_map, &trailer_map, info);
    auto value = response_header_status[CelValue::CreateStringView(GrpcStatus)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    EXPECT_EQ(0x7, value.value().Int64OrDie());
  }
  {
    Http::TestResponseHeaderMapImpl header_map{{header_name, "a"}};
    Http::TestResponseTrailerMapImpl trailer_map{{trailer_name, "b"}};
    ResponseWrapper response_no_status(&header_map, &trailer_map, info);
    auto value = response_no_status[CelValue::CreateStringView(GrpcStatus)];
    EXPECT_TRUE(value.has_value());
    ASSERT_TRUE(value.value().IsInt64());
    EXPECT_EQ(0xc, value.value().Int64OrDie()); // http:404 -> grpc:12
  }
  {
    NiceMock<StreamInfo::MockStreamInfo> info_without_code;
    Http::TestResponseHeaderMapImpl header_map{{header_name, "a"}};
    Http::TestResponseTrailerMapImpl trailer_map{{trailer_name, "b"}};
    ResponseWrapper response_no_status(&header_map, &trailer_map, info_without_code);
    auto value = response_no_status[CelValue::CreateStringView(GrpcStatus)];
    EXPECT_FALSE(value.has_value());
  }
}