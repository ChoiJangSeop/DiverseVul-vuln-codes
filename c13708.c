static int r_jwe_aesgcm_key_unwrap(jwe_t * jwe, jwa_alg alg, jwk_t * jwk, int x5u_flags) {
  int ret, res;
  unsigned char iv[96] = {0}, * key = NULL, cipherkey[64] = {0}, tag[128] = {0}, tag_b64url[256] = {0};
  size_t iv_len = 0, key_len = 0, cipherkey_len = 0, tag_b64url_len = 0, tag_len = gnutls_cipher_get_tag_size(r_jwe_get_alg_from_alg(alg));
  unsigned int bits = 0;
  gnutls_datum_t key_g, iv_g;
  gnutls_cipher_hd_t handle = NULL;

  if (r_jwk_key_type(jwk, &bits, x5u_flags) & R_KEY_TYPE_SYMMETRIC && o_strlen(r_jwe_get_header_str_value(jwe, "iv")) && o_strlen(r_jwe_get_header_str_value(jwe, "tag"))) {
    ret = RHN_OK;
    key_len = bits;

    do {
      if ((key = o_malloc(key_len+4)) == NULL) {
        y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Error allocating resources for key");
        ret = RHN_ERROR_MEMORY;
        break;
      }
      if (r_jwk_export_to_symmetric_key(jwk, key, &key_len) != RHN_OK) {
        y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Error r_jwk_export_to_symmetric_key");
        ret = RHN_ERROR;
        break;
      }
      if (!o_base64url_decode((const unsigned char *)r_jwe_get_header_str_value(jwe, "iv"), o_strlen(r_jwe_get_header_str_value(jwe, "iv")), iv, &iv_len)) {
        y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Error o_base64url_decode iv");
        ret = RHN_ERROR_INVALID;
        break;
      }
      if (!o_base64url_decode((const unsigned char *)jwe->encrypted_key_b64url, o_strlen((const char *)jwe->encrypted_key_b64url), cipherkey, &cipherkey_len)) {
        y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Error o_base64url_decode cipherkey");
        ret = RHN_ERROR_INVALID;
        break;
      }
      key_g.data = key;
      key_g.size = key_len;
      iv_g.data = iv;
      iv_g.size = iv_len;
      if ((res = gnutls_cipher_init(&handle, r_jwe_get_alg_from_alg(alg), &key_g, &iv_g))) {
        y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Error gnutls_cipher_init: '%s'", gnutls_strerror(res));
        ret = RHN_ERROR_INVALID;
        break;
      }
      if ((res = gnutls_cipher_decrypt(handle, cipherkey, cipherkey_len))) {
        y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Error gnutls_cipher_decrypt: '%s'", gnutls_strerror(res));
        ret = RHN_ERROR;
        break;
      }
      if ((res = gnutls_cipher_tag(handle, tag, tag_len))) {
        y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Error gnutls_cipher_tag: '%s'", gnutls_strerror(res));
        ret = RHN_ERROR;
        break;
      }
      if (!o_base64url_encode(tag, tag_len, tag_b64url, &tag_b64url_len)) {
        y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Error o_base64url_encode tag");
        ret = RHN_ERROR;
        break;
      }
      tag_b64url[tag_b64url_len] = '\0';
      if (0 != o_strcmp((const char *)tag_b64url, r_jwe_get_header_str_value(jwe, "tag"))) {
        y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Invalid tag %s %s", tag_b64url, r_jwe_get_header_str_value(jwe, "tag"));
        ret = RHN_ERROR_INVALID;
        break;
      }
      if (r_jwe_set_cypher_key(jwe, cipherkey, cipherkey_len) != RHN_OK) {
        y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Error r_jwe_set_cypher_key");
        ret = RHN_ERROR;
      }

    } while (0);
    o_free(key);
    if (handle != NULL) {
      gnutls_cipher_deinit(handle);
    }
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "r_jwe_aesgcm_key_unwrap - Error invalid key");
    ret = RHN_ERROR_INVALID;
  }
  return ret;
}