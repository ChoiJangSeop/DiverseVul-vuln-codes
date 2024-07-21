bool cephx_verify_authorizer(CephContext *cct, KeyStore *keys,
			     bufferlist::iterator& indata,
			     CephXServiceTicketInfo& ticket_info, bufferlist& reply_bl)
{
  __u8 authorizer_v;
  uint32_t service_id;
  uint64_t global_id;
  CryptoKey service_secret;
  // ticket blob
  CephXTicketBlob ticket;


  try {
    ::decode(authorizer_v, indata);
    ::decode(global_id, indata);
    ::decode(service_id, indata);
    ::decode(ticket, indata);
  } catch (buffer::end_of_buffer &e) {
    // Unable to decode!
    return false;
  }
  ldout(cct, 10) << "verify_authorizer decrypted service "
	   << ceph_entity_type_name(service_id)
	   << " secret_id=" << ticket.secret_id << dendl;

  if (ticket.secret_id == (uint64_t)-1) {
    EntityName name;
    name.set_type(service_id);
    if (!keys->get_secret(name, service_secret)) {
      ldout(cct, 0) << "verify_authorizer could not get general service secret for service "
	      << ceph_entity_type_name(service_id) << " secret_id=" << ticket.secret_id << dendl;
      return false;
    }
  } else {
    if (!keys->get_service_secret(service_id, ticket.secret_id, service_secret)) {
      ldout(cct, 0) << "verify_authorizer could not get service secret for service "
	      << ceph_entity_type_name(service_id) << " secret_id=" << ticket.secret_id << dendl;
      if (cct->_conf->auth_debug && ticket.secret_id == 0)
	assert(0 == "got secret_id=0");
      return false;
    }
  }
  std::string error;
  if (!service_secret.get_secret().length())
    error = "invalid key";  // Bad key?
  else
    decode_decrypt_enc_bl(cct, ticket_info, service_secret, ticket.blob, error);
  if (!error.empty()) {
    ldout(cct, 0) << "verify_authorizer could not decrypt ticket info: error: "
      << error << dendl;
    return false;
  }

  if (ticket_info.ticket.global_id != global_id) {
    ldout(cct, 0) << "verify_authorizer global_id mismatch: declared id=" << global_id
	    << " ticket_id=" << ticket_info.ticket.global_id << dendl;
    return false;
  }

  ldout(cct, 10) << "verify_authorizer global_id=" << global_id << dendl;

  // CephXAuthorize
  CephXAuthorize auth_msg;
  if (decode_decrypt(cct, auth_msg, ticket_info.session_key, indata, error)) {
    ldout(cct, 0) << "verify_authorizercould not decrypt authorize request with error: "
      << error << dendl;
    return false;
  }

  /*
   * Reply authorizer:
   *  {timestamp + 1}^session_key
   */
  CephXAuthorizeReply reply;
  // reply.trans_id = auth_msg.trans_id;
  reply.nonce_plus_one = auth_msg.nonce + 1;
  if (encode_encrypt(cct, reply, ticket_info.session_key, reply_bl, error)) {
    ldout(cct, 10) << "verify_authorizer: encode_encrypt error: " << error << dendl;
    return false;
  }

  ldout(cct, 10) << "verify_authorizer ok nonce " << hex << auth_msg.nonce << dec
	   << " reply_bl.length()=" << reply_bl.length() <<  dendl;
  return true;
}