napi_status napi_get_value_string_utf16(napi_env env,
                                        napi_value value,
                                        char16_t* buf,
                                        size_t bufsize,
                                        size_t* result) {
  CHECK_ENV(env);
  CHECK_ARG(env, value);

  v8::Local<v8::Value> val = v8impl::V8LocalValueFromJsValue(value);
  RETURN_STATUS_IF_FALSE(env, val->IsString(), napi_string_expected);

  if (!buf) {
    CHECK_ARG(env, result);
    // V8 assumes UTF-16 length is the same as the number of characters.
    *result = val.As<v8::String>()->Length();
  } else {
    int copied = val.As<v8::String>()->Write(env->isolate,
                                             reinterpret_cast<uint16_t*>(buf),
                                             0,
                                             bufsize - 1,
                                             v8::String::NO_NULL_TERMINATION);

    buf[copied] = '\0';
    if (result != nullptr) {
      *result = copied;
    }
  }

  return napi_clear_last_error(env);
}