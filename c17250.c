MaybeLocal<Object> X509ToObject(Environment* env, X509* cert) {
  EscapableHandleScope scope(env->isolate());
  Local<Context> context = env->context();
  Local<Object> info = Object::New(env->isolate());

  BIOPointer bio(BIO_new(BIO_s_mem()));

  if (!Set<Value>(context,
                  info,
                  env->subject_string(),
                  GetSubject(env, bio, cert)) ||
      !Set<Value>(context,
                  info,
                  env->issuer_string(),
                  GetIssuerString(env, bio, cert)) ||
      !Set<Value>(context,
                  info,
                  env->subjectaltname_string(),
                  GetSubjectAltNameString(env, bio, cert)) ||
      !Set<Value>(context,
                  info,
                  env->infoaccess_string(),
                  GetInfoAccessString(env, bio, cert))) {
    return MaybeLocal<Object>();
  }

  EVPKeyPointer pkey(X509_get_pubkey(cert));
  RSAPointer rsa;
  ECPointer ec;
  if (pkey) {
    switch (EVP_PKEY_id(pkey.get())) {
      case EVP_PKEY_RSA:
        rsa.reset(EVP_PKEY_get1_RSA(pkey.get()));
        break;
      case EVP_PKEY_EC:
        ec.reset(EVP_PKEY_get1_EC_KEY(pkey.get()));
        break;
    }
  }

  if (rsa) {
    const BIGNUM* n;
    const BIGNUM* e;
    RSA_get0_key(rsa.get(), &n, &e, nullptr);
    if (!Set<Value>(context,
                    info,
                    env->modulus_string(),
                    GetModulusString(env, bio, n)) ||
        !Set<Value>(context, info, env->bits_string(), GetBits(env, n)) ||
        !Set<Value>(context,
                    info,
                    env->exponent_string(),
                    GetExponentString(env, bio, e)) ||
        !Set<Object>(context,
                     info,
                     env->pubkey_string(),
                     GetPubKey(env, rsa))) {
      return MaybeLocal<Object>();
    }
  } else if (ec) {
    const EC_GROUP* group = EC_KEY_get0_group(ec.get());

    if (!Set<Value>(context,
                    info,
                    env->bits_string(),
                    GetECGroup(env, group, ec)) ||
        !Set<Value>(context,
                    info,
                    env->pubkey_string(),
                    GetECPubKey(env, group, ec))) {
      return MaybeLocal<Object>();
    }

    const int nid = EC_GROUP_get_curve_name(group);
    if (nid != 0) {
      // Curve is well-known, get its OID and NIST nick-name (if it has one).

      if (!Set<Value>(context,
                      info,
                      env->asn1curve_string(),
                      GetCurveASN1Name(env, nid)) ||
          !Set<Value>(context,
                      info,
                      env->nistcurve_string(),
                      GetCurveNistName(env, nid))) {
        return MaybeLocal<Object>();
      }
    } else {
      // Unnamed curves can be described by their mathematical properties,
      // but aren't used much (at all?) with X.509/TLS. Support later if needed.
    }
  }

  // pkey, rsa, and ec pointers are no longer needed.
  pkey.reset();
  rsa.reset();
  ec.reset();

  if (!Set<Value>(context,
                  info,
                  env->valid_from_string(),
                  GetValidFrom(env, cert, bio)) ||
      !Set<Value>(context,
                  info,
                  env->valid_to_string(),
                  GetValidTo(env, cert, bio))) {
    return MaybeLocal<Object>();
  }

  // bio is no longer needed
  bio.reset();

  if (!Set<Value>(context,
                  info,
                  env->fingerprint_string(),
                  GetFingerprintDigest(env, EVP_sha1(), cert)) ||
      !Set<Value>(context,
                  info,
                  env->fingerprint256_string(),
                  GetFingerprintDigest(env, EVP_sha256(), cert)) ||
      !Set<Value>(context,
                  info,
                  env->fingerprint512_string(),
                  GetFingerprintDigest(env, EVP_sha512(), cert)) ||
      !Set<Value>(context,
                  info,
                  env->ext_key_usage_string(),
                  GetKeyUsage(env, cert)) ||
      !Set<Value>(context,
                  info,
                  env->serial_number_string(),
                  GetSerialNumber(env, cert)) ||
      !Set<Object>(context,
                   info,
                   env->raw_string(),
                   GetRawDERCertificate(env, cert))) {
    return MaybeLocal<Object>();
  }

  return scope.Escape(info);
}