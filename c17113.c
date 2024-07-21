int StreamBase::Shutdown(v8::Local<v8::Object> req_wrap_obj) {
  Environment* env = stream_env();

  v8::HandleScope handle_scope(env->isolate());

  if (req_wrap_obj.IsEmpty()) {
    if (!env->shutdown_wrap_template()
             ->NewInstance(env->context())
             .ToLocal(&req_wrap_obj)) {
      return UV_EBUSY;
    }
    StreamReq::ResetObject(req_wrap_obj);
  }

  AsyncHooks::DefaultTriggerAsyncIdScope trigger_scope(GetAsyncWrap());
  ShutdownWrap* req_wrap = CreateShutdownWrap(req_wrap_obj);
  int err = DoShutdown(req_wrap);

  if (err != 0 && req_wrap != nullptr) {
    req_wrap->Dispose();
  }

  const char* msg = Error();
  if (msg != nullptr) {
    req_wrap_obj->Set(
        env->context(),
        env->error_string(), OneByteString(env->isolate(), msg)).Check();
    ClearError();
  }

  return err;
}