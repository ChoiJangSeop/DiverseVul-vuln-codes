void X509Certificate::ToLegacy(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);
  X509Certificate* cert;
  ASSIGN_OR_RETURN_UNWRAP(&cert, args.Holder());
  Local<Value> ret;
  if (X509ToObject(env, cert->get()).ToLocal(&ret))
    args.GetReturnValue().Set(ret);
}