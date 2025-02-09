int CephxServiceHandler::handle_request(bufferlist::iterator& indata, bufferlist& result_bl, uint64_t& global_id, AuthCapsInfo& caps, uint64_t *auid)
{
  int ret = 0;

  struct CephXRequestHeader cephx_header;
  ::decode(cephx_header, indata);


  switch (cephx_header.request_type) {
  case CEPHX_GET_AUTH_SESSION_KEY:
    {
      ldout(cct, 10) << "handle_request get_auth_session_key for " << entity_name << dendl;

      CephXAuthenticate req;
      ::decode(req, indata);

      CryptoKey secret;
      if (!key_server->get_secret(entity_name, secret)) {
        ldout(cct, 0) << "couldn't find entity name: " << entity_name << dendl;
	ret = -EPERM;
	break;
      }

      if (!server_challenge) {
	ret = -EPERM;
	break;
      }      

      uint64_t expected_key;
      std::string error;
      cephx_calc_client_server_challenge(cct, secret, server_challenge,
					 req.client_challenge, &expected_key, error);
      if (!error.empty()) {
	ldout(cct, 0) << " cephx_calc_client_server_challenge error: " << error << dendl;
	ret = -EPERM;
	break;
      }

      ldout(cct, 20) << " checking key: req.key=" << hex << req.key
	       << " expected_key=" << expected_key << dec << dendl;
      if (req.key != expected_key) {
        ldout(cct, 0) << " unexpected key: req.key=" << hex << req.key
		<< " expected_key=" << expected_key << dec << dendl;
        ret = -EPERM;
	break;
      }

      CryptoKey session_key;
      CephXSessionAuthInfo info;
      bool should_enc_ticket = false;

      EntityAuth eauth;
      if (! key_server->get_auth(entity_name, eauth)) {
	ret = -EPERM;
	break;
      }
      CephXServiceTicketInfo old_ticket_info;

      if (cephx_decode_ticket(cct, key_server, CEPH_ENTITY_TYPE_AUTH,
			      req.old_ticket, old_ticket_info)) {
        global_id = old_ticket_info.ticket.global_id;
        ldout(cct, 10) << "decoded old_ticket with global_id=" << global_id << dendl;
        should_enc_ticket = true;
      }

      info.ticket.init_timestamps(ceph_clock_now(), cct->_conf->auth_mon_ticket_ttl);
      info.ticket.name = entity_name;
      info.ticket.global_id = global_id;
      info.ticket.auid = eauth.auid;
      info.validity += cct->_conf->auth_mon_ticket_ttl;

      if (auid) *auid = eauth.auid;

      key_server->generate_secret(session_key);

      info.session_key = session_key;
      info.service_id = CEPH_ENTITY_TYPE_AUTH;
      if (!key_server->get_service_secret(CEPH_ENTITY_TYPE_AUTH, info.service_secret, info.secret_id)) {
        ldout(cct, 0) << " could not get service secret for auth subsystem" << dendl;
        ret = -EIO;
        break;
      }

      vector<CephXSessionAuthInfo> info_vec;
      info_vec.push_back(info);

      build_cephx_response_header(cephx_header.request_type, 0, result_bl);
      if (!cephx_build_service_ticket_reply(cct, eauth.key, info_vec, should_enc_ticket,
					    old_ticket_info.session_key, result_bl)) {
	ret = -EIO;
      }

      if (!key_server->get_service_caps(entity_name, CEPH_ENTITY_TYPE_MON, caps)) {
        ldout(cct, 0) << " could not get mon caps for " << entity_name << dendl;
        ret = -EACCES;
      } else {
        char *caps_str = caps.caps.c_str();
        if (!caps_str || !caps_str[0]) {
          ldout(cct,0) << "mon caps null for " << entity_name << dendl;
          ret = -EACCES;
        }
      }
    }
    break;

  case CEPHX_GET_PRINCIPAL_SESSION_KEY:
    {
      ldout(cct, 10) << "handle_request get_principal_session_key" << dendl;

      bufferlist tmp_bl;
      CephXServiceTicketInfo auth_ticket_info;
      if (!cephx_verify_authorizer(cct, key_server, indata, auth_ticket_info, tmp_bl)) {
        ret = -EPERM;
	break;
      }

      CephXServiceTicketRequest ticket_req;
      ::decode(ticket_req, indata);
      ldout(cct, 10) << " ticket_req.keys = " << ticket_req.keys << dendl;

      ret = 0;
      vector<CephXSessionAuthInfo> info_vec;
      int found_services = 0;
      int service_err = 0;
      for (uint32_t service_id = 1; service_id <= ticket_req.keys;
	   service_id <<= 1) {
        if (ticket_req.keys & service_id) {
	  ldout(cct, 10) << " adding key for service "
			 << ceph_entity_type_name(service_id) << dendl;
          CephXSessionAuthInfo info;
          int r = key_server->build_session_auth_info(service_id,
						      auth_ticket_info, info);
	  // tolerate missing MGR rotating key for the purposes of upgrades.
          if (r < 0) {
	    ldout(cct, 10) << "   missing key for service "
			   << ceph_entity_type_name(service_id) << dendl;
	    service_err = r;
	    continue;
	  }
          info.validity += cct->_conf->auth_service_ticket_ttl;
          info_vec.push_back(info);
	  ++found_services;
        }
      }
      if (!found_services && service_err) {
	ldout(cct, 10) << __func__ << " did not find any service keys" << dendl;
	ret = service_err;
      }
      CryptoKey no_key;
      build_cephx_response_header(cephx_header.request_type, ret, result_bl);
      cephx_build_service_ticket_reply(cct, auth_ticket_info.session_key, info_vec, false, no_key, result_bl);
    }
    break;

  case CEPHX_GET_ROTATING_KEY:
    {
      ldout(cct, 10) << "handle_request getting rotating secret for " << entity_name << dendl;
      build_cephx_response_header(cephx_header.request_type, 0, result_bl);
      if (!key_server->get_rotating_encrypted(entity_name, result_bl)) {
        ret = -EPERM;
        break;
      }
    }
    break;

  default:
    ldout(cct, 10) << "handle_request unknown op " << cephx_header.request_type << dendl;
    return -EINVAL;
  }
  return ret;
}