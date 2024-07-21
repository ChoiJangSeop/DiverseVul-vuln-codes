bool OSD::ms_verify_authorizer(Connection *con, int peer_type,
			       int protocol, bufferlist& authorizer_data, bufferlist& authorizer_reply,
			       bool& isvalid, CryptoKey& session_key)
{
  AuthAuthorizeHandler *authorize_handler = 0;
  switch (peer_type) {
  case CEPH_ENTITY_TYPE_MDS:
    /*
     * note: mds is technically a client from our perspective, but
     * this makes the 'cluster' consistent w/ monitor's usage.
     */
  case CEPH_ENTITY_TYPE_OSD:
  case CEPH_ENTITY_TYPE_MGR:
    authorize_handler = authorize_handler_cluster_registry->get_handler(protocol);
    break;
  default:
    authorize_handler = authorize_handler_service_registry->get_handler(protocol);
  }
  if (!authorize_handler) {
    dout(0) << "No AuthAuthorizeHandler found for protocol " << protocol << dendl;
    isvalid = false;
    return true;
  }

  AuthCapsInfo caps_info;
  EntityName name;
  uint64_t global_id;
  uint64_t auid = CEPH_AUTH_UID_DEFAULT;

  RotatingKeyRing *keys = monc->rotating_secrets.get();
  if (keys) {
    isvalid = authorize_handler->verify_authorizer(
      cct, keys,
      authorizer_data, authorizer_reply, name, global_id, caps_info, session_key,
      &auid);
  } else {
    dout(10) << __func__ << " no rotating_keys (yet), denied" << dendl;
    isvalid = false;
  }

  if (isvalid) {
    Session *s = static_cast<Session *>(con->get_priv());
    if (!s) {
      s = new Session(cct);
      con->set_priv(s->get());
      s->con = con;
      dout(10) << " new session " << s << " con=" << s->con << " addr=" << s->con->get_peer_addr() << dendl;
    }

    s->entity_name = name;
    if (caps_info.allow_all)
      s->caps.set_allow_all();
    s->auid = auid;

    if (caps_info.caps.length() > 0) {
      bufferlist::iterator p = caps_info.caps.begin();
      string str;
      try {
	::decode(str, p);
      }
      catch (buffer::error& e) {
      }
      bool success = s->caps.parse(str);
      if (success)
	dout(10) << " session " << s << " " << s->entity_name << " has caps " << s->caps << " '" << str << "'" << dendl;
      else
	dout(10) << " session " << s << " " << s->entity_name << " failed to parse caps '" << str << "'" << dendl;
    }

    s->put();
  }
  return true;
}