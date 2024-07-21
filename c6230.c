bool AuthUnknownAuthorizeHandler::verify_authorizer(CephContext *cct, KeyStore *keys,
						 bufferlist& authorizer_data, bufferlist& authorizer_reply,
						 EntityName& entity_name, uint64_t& global_id, AuthCapsInfo& caps_info, CryptoKey& session_key,
uint64_t *auid)
{
  // For unknown authorizers, there's nothing to verify.  They're "OK" by definition.  PLR

  return true;
}