int StreamBase::WriteString(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);
  CHECK(args[0]->IsObject());
  CHECK(args[1]->IsString());

  Local<Object> req_wrap_obj = args[0].As<Object>();
  Local<String> string = args[1].As<String>();
  Local<Object> send_handle_obj;
  if (args[2]->IsObject())
    send_handle_obj = args[2].As<Object>();

  // Compute the size of the storage that the string will be flattened into.
  // For UTF8 strings that are very long, go ahead and take the hit for
  // computing their actual size, rather than tripling the storage.
  size_t storage_size;
  if (enc == UTF8 && string->Length() > 65535 &&
      !StringBytes::Size(env->isolate(), string, enc).To(&storage_size))
    return 0;
  else if (!StringBytes::StorageSize(env->isolate(), string, enc)
                .To(&storage_size))
    return 0;

  if (storage_size > INT_MAX)
    return UV_ENOBUFS;

  // Try writing immediately if write size isn't too big
  char stack_storage[16384];  // 16kb
  size_t data_size;
  size_t synchronously_written = 0;
  uv_buf_t buf;

  bool try_write = storage_size <= sizeof(stack_storage) &&
                   (!IsIPCPipe() || send_handle_obj.IsEmpty());
  if (try_write) {
    data_size = StringBytes::Write(env->isolate(),
                                   stack_storage,
                                   storage_size,
                                   string,
                                   enc);
    buf = uv_buf_init(stack_storage, data_size);

    uv_buf_t* bufs = &buf;
    size_t count = 1;
    const int err = DoTryWrite(&bufs, &count);
    // Keep track of the bytes written here, because we're taking a shortcut
    // by using `DoTryWrite()` directly instead of using the utilities
    // provided by `Write()`.
    synchronously_written = count == 0 ? data_size : data_size - buf.len;
    bytes_written_ += synchronously_written;

    // Immediate failure or success
    if (err != 0 || count == 0) {
      SetWriteResult(StreamWriteResult { false, err, nullptr, data_size });
      return err;
    }

    // Partial write
    CHECK_EQ(count, 1);
  }

  AllocatedBuffer data;

  if (try_write) {
    // Copy partial data
    data = AllocatedBuffer::AllocateManaged(env, buf.len);
    memcpy(data.data(), buf.base, buf.len);
    data_size = buf.len;
  } else {
    // Write it
    data = AllocatedBuffer::AllocateManaged(env, storage_size);
    data_size = StringBytes::Write(env->isolate(),
                                   data.data(),
                                   storage_size,
                                   string,
                                   enc);
  }

  CHECK_LE(data_size, storage_size);

  buf = uv_buf_init(data.data(), data_size);

  uv_stream_t* send_handle = nullptr;

  if (IsIPCPipe() && !send_handle_obj.IsEmpty()) {
    HandleWrap* wrap;
    ASSIGN_OR_RETURN_UNWRAP(&wrap, send_handle_obj, UV_EINVAL);
    send_handle = reinterpret_cast<uv_stream_t*>(wrap->GetHandle());
    // Reference LibuvStreamWrap instance to prevent it from being garbage
    // collected before `AfterWrite` is called.
    req_wrap_obj->Set(env->context(),
                      env->handle_string(),
                      send_handle_obj).Check();
  }

  StreamWriteResult res = Write(&buf, 1, send_handle, req_wrap_obj);
  res.bytes += synchronously_written;

  SetWriteResult(res);
  if (res.wrap != nullptr) {
    res.wrap->SetAllocatedStorage(std::move(data));
  }

  return res.err;
}