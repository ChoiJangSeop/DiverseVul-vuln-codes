v8::MaybeLocal<v8::Value> GetInfoString(
    Environment* env,
    const BIOPointer& bio,
    X509* cert) {
  int index = X509_get_ext_by_NID(cert, nid, -1);
  if (index < 0)
    return Undefined(env->isolate());

  X509_EXTENSION* ext = X509_get_ext(cert, index);
  CHECK_NOT_NULL(ext);

  if (!SafeX509ExtPrint(bio, ext) &&
      X509V3_EXT_print(bio.get(), ext, 0, 0) != 1) {
    USE(BIO_reset(bio.get()));
    return v8::Null(env->isolate());
  }

  return ToV8Value(env, bio);
}