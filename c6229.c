CephXAuthorizer *CephXTicketHandler::build_authorizer(uint64_t global_id) const
{
  CephXAuthorizer *a = new CephXAuthorizer(cct);
  a->session_key = session_key;
  a->nonce = ((uint64_t)rand() << 32) + rand();

  __u8 authorizer_v = 1;
  ::encode(authorizer_v, a->bl);
  ::encode(global_id, a->bl);
  ::encode(service_id, a->bl);

  ::encode(ticket, a->bl);

  CephXAuthorize msg;
  msg.nonce = a->nonce;

  std::string error;
  if (encode_encrypt(cct, msg, session_key, a->bl, error)) {
    ldout(cct, 0) << "failed to encrypt authorizer: " << error << dendl;
    delete a;
    return 0;
  }
  return a;
}