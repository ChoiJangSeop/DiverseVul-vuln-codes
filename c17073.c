napi_status napi_get_value_string_utf8(napi_env env,
                                       napi_value value,
                                       char* buf,
                                       size_t bufsize,
                                       size_t* result) {
  CHECK_ENV(env);
  CHECK_ARG(env, value);

  v8::Local<v8::Value> val = v8impl::V8LocalValueFromJsValue(value);
  RETURN_STATUS_IF_FALSE(env, val->IsString(), napi_string_expected);

  if (!buf) {
    CHECK_ARG(env, result);
    *result = val.As<v8::String>()->Utf8Length(env->isolate);
  } else {
    int copied = val.As<v8::String>()->WriteUtf8(
        env->isolate,
        buf,
        bufsize - 1,
        nullptr,
        v8::String::REPLACE_INVALID_UTF8 | v8::String::NO_NULL_TERMINATION);

    buf[copied] = '\0';
    if (result != nullptr) {
      *result = copied;
    }
  }

  return napi_clear_last_error(env);
}