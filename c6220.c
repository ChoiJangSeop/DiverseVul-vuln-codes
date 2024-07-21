bool DaemonServer::ms_verify_authorizer(Connection *con,
    int peer_type,
    int protocol,
    ceph::bufferlist& authorizer_data,
    ceph::bufferlist& authorizer_reply,
    bool& is_valid,
    CryptoKey& session_key)
{
  AuthAuthorizeHandler *handler = nullptr;
  if (peer_type == CEPH_ENTITY_TYPE_OSD ||
      peer_type == CEPH_ENTITY_TYPE_MON ||
      peer_type == CEPH_ENTITY_TYPE_MDS ||
      peer_type == CEPH_ENTITY_TYPE_MGR) {
    handler = auth_cluster_registry.get_handler(protocol);
  } else {
    handler = auth_service_registry.get_handler(protocol);
  }
  if (!handler) {
    dout(0) << "No AuthAuthorizeHandler found for protocol " << protocol << dendl;
    is_valid = false;
    return true;
  }

  MgrSessionRef s(new MgrSession(cct));
  s->inst.addr = con->get_peer_addr();
  AuthCapsInfo caps_info;

  RotatingKeyRing *keys = monc->rotating_secrets.get();
  if (keys) {
    is_valid = handler->verify_authorizer(
      cct, keys,
      authorizer_data,
      authorizer_reply, s->entity_name,
      s->global_id, caps_info,
      session_key);
  } else {
    dout(10) << __func__ << " no rotating_keys (yet), denied" << dendl;
    is_valid = false;
  }

  if (is_valid) {
    if (caps_info.allow_all) {
      dout(10) << " session " << s << " " << s->entity_name
	       << " allow_all" << dendl;
      s->caps.set_allow_all();
    }
    if (caps_info.caps.length() > 0) {
      bufferlist::iterator p = caps_info.caps.begin();
      string str;
      try {
	::decode(str, p);
      }
      catch (buffer::error& e) {
      }
      bool success = s->caps.parse(str);
      if (success) {
	dout(10) << " session " << s << " " << s->entity_name
		 << " has caps " << s->caps << " '" << str << "'" << dendl;
      } else {
	dout(10) << " session " << s << " " << s->entity_name
		 << " failed to parse caps '" << str << "'" << dendl;
	is_valid = false;
      }
    }
    con->set_priv(s->get());

    if (peer_type == CEPH_ENTITY_TYPE_OSD) {
      Mutex::Locker l(lock);
      s->osd_id = atoi(s->entity_name.get_id().c_str());
      dout(10) << "registering osd." << s->osd_id << " session "
	       << s << " con " << con << dendl;
      osd_cons[s->osd_id].insert(con);
    }
  }

  return true;
}