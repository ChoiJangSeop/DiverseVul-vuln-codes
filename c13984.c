bool Crypto::get_random_values(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  if (!args.requireAtLeast(cx, "crypto.getRandomValues", 1))
    return false;

  if (!args[0].isObject() || !is_int_typed_array(&args[0].toObject())) {
    JS_ReportErrorUTF8(cx, "crypto.getRandomValues: input must be an integer-typed TypedArray");
    return false;
  }

  JS::RootedObject typed_array(cx, &args[0].toObject());
  size_t byte_length = JS_GetArrayBufferViewByteLength(typed_array);
  if (byte_length > MAX_BYTE_LENGTH) {
    JS_ReportErrorUTF8(cx,
                       "crypto.getRandomValues: input byteLength must be at most %u, "
                       "but is %zu",
                       MAX_BYTE_LENGTH, byte_length);
    return false;
  }

  JS::AutoCheckCannotGC noGC(cx);
  bool is_shared;
  void *buffer = JS_GetArrayBufferViewData(typed_array, &is_shared, noGC);
  arc4random_buf(buffer, byte_length);

  args.rval().setObject(*typed_array);
  return true;
}