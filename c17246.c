void X509Certificate::InfoAccess(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);
  X509Certificate* cert;
  ASSIGN_OR_RETURN_UNWRAP(&cert, args.Holder());
  BIOPointer bio(BIO_new(BIO_s_mem()));
  Local<Value> ret;
  if (GetInfoString<NID_info_access>(env, bio, cert->get()).ToLocal(&ret))
    args.GetReturnValue().Set(ret);
}