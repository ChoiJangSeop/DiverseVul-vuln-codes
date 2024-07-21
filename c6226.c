bool SimpleMessenger::verify_authorizer(Connection *con, int peer_type,
					int protocol, bufferlist& authorizer, bufferlist& authorizer_reply,
					bool& isvalid,CryptoKey& session_key)
{
  return ms_deliver_verify_authorizer(con, peer_type, protocol, authorizer, authorizer_reply, isvalid,session_key);
}