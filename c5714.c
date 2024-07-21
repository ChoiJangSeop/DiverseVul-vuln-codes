CURLcode Curl_ntlm_core_mk_ntlmv2_hash(const char *user, size_t userlen,
                                       const char *domain, size_t domlen,
                                       unsigned char *ntlmhash,
                                       unsigned char *ntlmv2hash)
{
  /* Unicode representation */
  size_t identity_len = (userlen + domlen) * 2;
  unsigned char *identity = malloc(identity_len);
  CURLcode result = CURLE_OK;

  if(!identity)
    return CURLE_OUT_OF_MEMORY;

  ascii_uppercase_to_unicode_le(identity, user, userlen);
  ascii_to_unicode_le(identity + (userlen << 1), domain, domlen);

  result = Curl_hmac_md5(ntlmhash, 16, identity, curlx_uztoui(identity_len),
                         ntlmv2hash);

  free(identity);

  return result;
}