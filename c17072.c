napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
    DECLARE_NAPI_PROPERTY("TestLatin1", TestLatin1),
    DECLARE_NAPI_PROPERTY("TestLatin1Insufficient", TestLatin1Insufficient),
    DECLARE_NAPI_PROPERTY("TestUtf8", TestUtf8),
    DECLARE_NAPI_PROPERTY("TestUtf8Insufficient", TestUtf8Insufficient),
    DECLARE_NAPI_PROPERTY("TestUtf16", TestUtf16),
    DECLARE_NAPI_PROPERTY("TestUtf16Insufficient", TestUtf16Insufficient),
    DECLARE_NAPI_PROPERTY("Utf16Length", Utf16Length),
    DECLARE_NAPI_PROPERTY("Utf8Length", Utf8Length),
    DECLARE_NAPI_PROPERTY("TestLargeUtf8", TestLargeUtf8),
    DECLARE_NAPI_PROPERTY("TestLargeLatin1", TestLargeLatin1),
    DECLARE_NAPI_PROPERTY("TestLargeUtf16", TestLargeUtf16),
  };

  NAPI_CALL(env, napi_define_properties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}