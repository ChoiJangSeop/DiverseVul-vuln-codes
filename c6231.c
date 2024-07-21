bool MDSDaemon::ms_verify_authorizer(Connection *con, int peer_type,
			       int protocol, bufferlist& authorizer_data, bufferlist& authorizer_reply,
			       bool& is_valid, CryptoKey& session_key)
{
  Mutex::Locker l(mds_lock);
  if (stopping) {
    return false;
  }
  if (beacon.get_want_state() == CEPH_MDS_STATE_DNE)
    return false;

  AuthAuthorizeHandler *authorize_handler = 0;
  switch (peer_type) {
  case CEPH_ENTITY_TYPE_MDS:
    authorize_handler = authorize_handler_cluster_registry->get_handler(protocol);
    break;
  default:
    authorize_handler = authorize_handler_service_registry->get_handler(protocol);
  }
  if (!authorize_handler) {
    dout(0) << "No AuthAuthorizeHandler found for protocol " << protocol << dendl;
    is_valid = false;
    return true;
  }

  AuthCapsInfo caps_info;
  EntityName name;
  uint64_t global_id;

  RotatingKeyRing *keys = monc->rotating_secrets.get();
  if (keys) {
    is_valid = authorize_handler->verify_authorizer(
      cct, keys,
      authorizer_data, authorizer_reply, name, global_id, caps_info,
      session_key);
  } else {
    dout(10) << __func__ << " no rotating_keys (yet), denied" << dendl;
    is_valid = false;
  }

  if (is_valid) {
    entity_name_t n(con->get_peer_type(), global_id);

    // We allow connections and assign Session instances to connections
    // even if we have not been assigned a rank, because clients with
    // "allow *" are allowed to connect and do 'tell' operations before
    // we have a rank.
    Session *s = NULL;
    if (mds_rank) {
      // If we do hold a rank, see if this is an existing client establishing
      // a new connection, rather than a new client
      s = mds_rank->sessionmap.get_session(n);
    }

    // Wire up a Session* to this connection
    // It doesn't go into a SessionMap instance until it sends an explicit
    // request to open a session (initial state of Session is `closed`)
    if (!s) {
      s = new Session;
      s->info.auth_name = name;
      s->info.inst.addr = con->get_peer_addr();
      s->info.inst.name = n;
      dout(10) << " new session " << s << " for " << s->info.inst << " con " << con << dendl;
      con->set_priv(s);
      s->connection = con;
    } else {
      dout(10) << " existing session " << s << " for " << s->info.inst << " existing con " << s->connection
	       << ", new/authorizing con " << con << dendl;
      con->set_priv(s->get());



      // Wait until we fully accept the connection before setting
      // s->connection.  In particular, if there are multiple incoming
      // connection attempts, they will all get their authorizer
      // validated, but some of them may "lose the race" and get
      // dropped.  We only want to consider the winner(s).  See
      // ms_handle_accept().  This is important for Sessions we replay
      // from the journal on recovery that don't have established
      // messenger state; we want the con from only the winning
      // connect attempt(s).  (Normal reconnects that don't follow MDS
      // recovery are reconnected to the existing con by the
      // messenger.)
    }

    if (caps_info.allow_all) {
      // Flag for auth providers that don't provide cap strings
      s->auth_caps.set_allow_all();
    } else {
      bufferlist::iterator p = caps_info.caps.begin();
      string auth_cap_str;
      try {
        ::decode(auth_cap_str, p);

        dout(10) << __func__ << ": parsing auth_cap_str='" << auth_cap_str << "'" << dendl;
        std::ostringstream errstr;
        if (!s->auth_caps.parse(g_ceph_context, auth_cap_str, &errstr)) {
          dout(1) << __func__ << ": auth cap parse error: " << errstr.str()
		  << " parsing '" << auth_cap_str << "'" << dendl;
	  clog->warn() << name << " mds cap '" << auth_cap_str
		       << "' does not parse: " << errstr.str();
          is_valid = false;
        }
      } catch (buffer::error& e) {
        // Assume legacy auth, defaults to:
        //  * permit all filesystem ops
        //  * permit no `tell` ops
        dout(1) << __func__ << ": cannot decode auth caps bl of length " << caps_info.caps.length() << dendl;
        is_valid = false;
      }
    }
  }

  return true;  // we made a decision (see is_valid)
}