int Pipe::accept()
{
  ldout(msgr->cct,10) << "accept" << dendl;
  assert(pipe_lock.is_locked());
  assert(state == STATE_ACCEPTING);

  pipe_lock.Unlock();

  // vars
  bufferlist addrs;
  entity_addr_t socket_addr;
  socklen_t len;
  int r;
  char banner[strlen(CEPH_BANNER)+1];
  bufferlist addrbl;
  ceph_msg_connect connect;
  ceph_msg_connect_reply reply;
  Pipe *existing = 0;
  bufferptr bp;
  bufferlist authorizer, authorizer_reply;
  bool authorizer_valid;
  uint64_t feat_missing;
  bool replaced = false;
  // this variable denotes if the connection attempt from peer is a hard 
  // reset or not, it is true if there is an existing connection and the
  // connection sequence from peer is equal to zero
  bool is_reset_from_peer = false;
  CryptoKey session_key;
  int removed; // single-use down below

  // this should roughly mirror pseudocode at
  //  http://ceph.com/wiki/Messaging_protocol
  int reply_tag = 0;
  uint64_t existing_seq = -1;

  // used for reading in the remote acked seq on connect
  uint64_t newly_acked_seq = 0;

  recv_reset();

  set_socket_options();

  // announce myself.
  r = tcp_write(CEPH_BANNER, strlen(CEPH_BANNER));
  if (r < 0) {
    ldout(msgr->cct,10) << "accept couldn't write banner" << dendl;
    goto fail_unlocked;
  }

  // and my addr
  ::encode(msgr->my_inst.addr, addrs, 0);  // legacy

  port = msgr->my_inst.addr.get_port();

  // and peer's socket addr (they might not know their ip)
  sockaddr_storage ss;
  len = sizeof(ss);
  r = ::getpeername(sd, (sockaddr*)&ss, &len);
  if (r < 0) {
    ldout(msgr->cct,0) << "accept failed to getpeername " << cpp_strerror(errno) << dendl;
    goto fail_unlocked;
  }
  socket_addr.set_sockaddr((sockaddr*)&ss);
  ::encode(socket_addr, addrs, 0);  // legacy

  r = tcp_write(addrs.c_str(), addrs.length());
  if (r < 0) {
    ldout(msgr->cct,10) << "accept couldn't write my+peer addr" << dendl;
    goto fail_unlocked;
  }

  ldout(msgr->cct,1) << "accept sd=" << sd << " " << socket_addr << dendl;
  
  // identify peer
  if (tcp_read(banner, strlen(CEPH_BANNER)) < 0) {
    ldout(msgr->cct,10) << "accept couldn't read banner" << dendl;
    goto fail_unlocked;
  }
  if (memcmp(banner, CEPH_BANNER, strlen(CEPH_BANNER))) {
    banner[strlen(CEPH_BANNER)] = 0;
    ldout(msgr->cct,1) << "accept peer sent bad banner '" << banner << "' (should be '" << CEPH_BANNER << "')" << dendl;
    goto fail_unlocked;
  }
  {
    bufferptr tp(sizeof(ceph_entity_addr));
    addrbl.push_back(std::move(tp));
  }
  if (tcp_read(addrbl.c_str(), addrbl.length()) < 0) {
    ldout(msgr->cct,10) << "accept couldn't read peer_addr" << dendl;
    goto fail_unlocked;
  }
  {
    bufferlist::iterator ti = addrbl.begin();
    ::decode(peer_addr, ti);
  }

  ldout(msgr->cct,10) << "accept peer addr is " << peer_addr << dendl;
  if (peer_addr.is_blank_ip()) {
    // peer apparently doesn't know what ip they have; figure it out for them.
    int port = peer_addr.get_port();
    peer_addr.u = socket_addr.u;
    peer_addr.set_port(port);
    ldout(msgr->cct,0) << "accept peer addr is really " << peer_addr
	    << " (socket is " << socket_addr << ")" << dendl;
  }
  set_peer_addr(peer_addr);  // so that connection_state gets set up
  
  while (1) {
    if (tcp_read((char*)&connect, sizeof(connect)) < 0) {
      ldout(msgr->cct,10) << "accept couldn't read connect" << dendl;
      goto fail_unlocked;
    }

    authorizer.clear();
    if (connect.authorizer_len) {
      bp = buffer::create(connect.authorizer_len);
      if (tcp_read(bp.c_str(), connect.authorizer_len) < 0) {
        ldout(msgr->cct,10) << "accept couldn't read connect authorizer" << dendl;
        goto fail_unlocked;
      }
      authorizer.push_back(std::move(bp));
      authorizer_reply.clear();
    }

    ldout(msgr->cct,20) << "accept got peer connect_seq " << connect.connect_seq
	     << " global_seq " << connect.global_seq
	     << dendl;
    
    msgr->lock.Lock();   // FIXME
    pipe_lock.Lock();
    if (msgr->dispatch_queue.stop)
      goto shutting_down;
    if (state != STATE_ACCEPTING) {
      goto shutting_down;
    }

    // note peer's type, flags
    set_peer_type(connect.host_type);
    policy = msgr->get_policy(connect.host_type);
    ldout(msgr->cct,10) << "accept of host_type " << connect.host_type
			<< ", policy.lossy=" << policy.lossy
			<< " policy.server=" << policy.server
			<< " policy.standby=" << policy.standby
			<< " policy.resetcheck=" << policy.resetcheck
			<< dendl;

    memset(&reply, 0, sizeof(reply));
    reply.protocol_version = msgr->get_proto_version(peer_type, false);
    msgr->lock.Unlock();

    // mismatch?
    ldout(msgr->cct,10) << "accept my proto " << reply.protocol_version
	     << ", their proto " << connect.protocol_version << dendl;
    if (connect.protocol_version != reply.protocol_version) {
      reply.tag = CEPH_MSGR_TAG_BADPROTOVER;
      goto reply;
    }

    // require signatures for cephx?
    if (connect.authorizer_protocol == CEPH_AUTH_CEPHX) {
      if (peer_type == CEPH_ENTITY_TYPE_OSD ||
	  peer_type == CEPH_ENTITY_TYPE_MDS ||
	  peer_type == CEPH_ENTITY_TYPE_MGR) {
	if (msgr->cct->_conf->cephx_require_signatures ||
	    msgr->cct->_conf->cephx_cluster_require_signatures) {
	  ldout(msgr->cct,10) << "using cephx, requiring MSG_AUTH feature bit for cluster" << dendl;
	  policy.features_required |= CEPH_FEATURE_MSG_AUTH;
	}
	if (msgr->cct->_conf->cephx_require_version >= 2 ||
	    msgr->cct->_conf->cephx_cluster_require_version >= 2) {
	  ldout(msgr->cct,10) << "using cephx, requiring cephx v2 feature bit for cluster" << dendl;
	  policy.features_required |= CEPH_FEATUREMASK_CEPHX_V2;
	}
      } else {
	if (msgr->cct->_conf->cephx_require_signatures ||
	    msgr->cct->_conf->cephx_service_require_signatures) {
	  ldout(msgr->cct,10) << "using cephx, requiring MSG_AUTH feature bit for service" << dendl;
	  policy.features_required |= CEPH_FEATURE_MSG_AUTH;
	}
	if (msgr->cct->_conf->cephx_require_version >= 2 ||
	    msgr->cct->_conf->cephx_service_require_version >= 2) {
	  ldout(msgr->cct,10) << "using cephx, requiring cephx v2 feature bit for cluster" << dendl;
	  policy.features_required |= CEPH_FEATUREMASK_CEPHX_V2;
	}
      }
    }

    feat_missing = policy.features_required & ~(uint64_t)connect.features;
    if (feat_missing) {
      ldout(msgr->cct,1) << "peer missing required features " << std::hex << feat_missing << std::dec << dendl;
      reply.tag = CEPH_MSGR_TAG_FEATURES;
      goto reply;
    }
    
    // Check the authorizer.  If not good, bail out.

    pipe_lock.Unlock();

    if (!msgr->verify_authorizer(connection_state.get(), peer_type, connect.authorizer_protocol, authorizer,
				 authorizer_reply, authorizer_valid, session_key) ||
	!authorizer_valid) {
      ldout(msgr->cct,0) << "accept: got bad authorizer" << dendl;
      pipe_lock.Lock();
      if (state != STATE_ACCEPTING)
	goto shutting_down_msgr_unlocked;
      reply.tag = CEPH_MSGR_TAG_BADAUTHORIZER;
      session_security.reset();
      goto reply;
    } 

    // We've verified the authorizer for this pipe, so set up the session security structure.  PLR

    ldout(msgr->cct,10) << "accept:  setting up session_security." << dendl;

  retry_existing_lookup:
    msgr->lock.Lock();
    pipe_lock.Lock();
    if (msgr->dispatch_queue.stop)
      goto shutting_down;
    if (state != STATE_ACCEPTING)
      goto shutting_down;
    
    // existing?
    existing = msgr->_lookup_pipe(peer_addr);
    if (existing) {
      existing->pipe_lock.Lock(true);  // skip lockdep check (we are locking a second Pipe here)
      if (existing->reader_dispatching) {
	/** we need to wait, or we can deadlock if downstream
	 *  fast_dispatchers are (naughtily!) waiting on resources
	 *  held by somebody trying to make use of the SimpleMessenger lock.
	 *  So drop locks, wait, and retry. It just looks like a slow network
	 *  to everybody else.
	 *
	 *  We take a ref to existing here since it might get reaped before we
	 *  wake up (see bug #15870).  We can be confident that it lived until
	 *  locked it since we held the msgr lock from _lookup_pipe through to
	 *  locking existing->lock and checking reader_dispatching.
	 */
	existing->get();
	pipe_lock.Unlock();
	msgr->lock.Unlock();
	existing->notify_on_dispatch_done = true;
	while (existing->reader_dispatching)
	  existing->cond.Wait(existing->pipe_lock);
	existing->pipe_lock.Unlock();
	existing->put();
	existing = nullptr;
	goto retry_existing_lookup;
      }

      if (connect.global_seq < existing->peer_global_seq) {
	ldout(msgr->cct,10) << "accept existing " << existing << ".gseq " << existing->peer_global_seq
		 << " > " << connect.global_seq << ", RETRY_GLOBAL" << dendl;
	reply.tag = CEPH_MSGR_TAG_RETRY_GLOBAL;
	reply.global_seq = existing->peer_global_seq;  // so we can send it below..
	existing->pipe_lock.Unlock();
	msgr->lock.Unlock();
	goto reply;
      } else {
	ldout(msgr->cct,10) << "accept existing " << existing << ".gseq " << existing->peer_global_seq
		 << " <= " << connect.global_seq << ", looks ok" << dendl;
      }
      
      if (existing->policy.lossy) {
	ldout(msgr->cct,0) << "accept replacing existing (lossy) channel (new one lossy="
	        << policy.lossy << ")" << dendl;
	existing->was_session_reset();
	goto replace;
      }

      ldout(msgr->cct,0) << "accept connect_seq " << connect.connect_seq
			 << " vs existing " << existing->connect_seq
			 << " state " << existing->get_state_name() << dendl;

      if (connect.connect_seq == 0 && existing->connect_seq > 0) {
	ldout(msgr->cct,0) << "accept peer reset, then tried to connect to us, replacing" << dendl;
        // this is a hard reset from peer
        is_reset_from_peer = true;
	if (policy.resetcheck)
	  existing->was_session_reset(); // this resets out_queue, msg_ and connect_seq #'s
	goto replace;
      }

      if (connect.connect_seq < existing->connect_seq) {
	// old attempt, or we sent READY but they didn't get it.
	ldout(msgr->cct,10) << "accept existing " << existing << ".cseq " << existing->connect_seq
			    << " > " << connect.connect_seq << ", RETRY_SESSION" << dendl;
	goto retry_session;
      }

      if (connect.connect_seq == existing->connect_seq) {
	// if the existing connection successfully opened, and/or
	// subsequently went to standby, then the peer should bump
	// their connect_seq and retry: this is not a connection race
	// we need to resolve here.
	if (existing->state == STATE_OPEN ||
	    existing->state == STATE_STANDBY) {
	  ldout(msgr->cct,10) << "accept connection race, existing " << existing
			      << ".cseq " << existing->connect_seq
			      << " == " << connect.connect_seq
			      << ", OPEN|STANDBY, RETRY_SESSION" << dendl;
	  goto retry_session;
	}

	// connection race?
	if (peer_addr < msgr->my_inst.addr ||
	    existing->policy.server) {
	  // incoming wins
	  ldout(msgr->cct,10) << "accept connection race, existing " << existing << ".cseq " << existing->connect_seq
		   << " == " << connect.connect_seq << ", or we are server, replacing my attempt" << dendl;
	  if (!(existing->state == STATE_CONNECTING ||
		existing->state == STATE_WAIT))
	    lderr(msgr->cct) << "accept race bad state, would replace, existing="
			     << existing->get_state_name()
			     << " " << existing << ".cseq=" << existing->connect_seq
			     << " == " << connect.connect_seq
			     << dendl;
	  assert(existing->state == STATE_CONNECTING ||
		 existing->state == STATE_WAIT);
	  goto replace;
	} else {
	  // our existing outgoing wins
	  ldout(msgr->cct,10) << "accept connection race, existing " << existing << ".cseq " << existing->connect_seq
		   << " == " << connect.connect_seq << ", sending WAIT" << dendl;
	  assert(peer_addr > msgr->my_inst.addr);
	  if (!(existing->state == STATE_CONNECTING))
	    lderr(msgr->cct) << "accept race bad state, would send wait, existing="
			     << existing->get_state_name()
			     << " " << existing << ".cseq=" << existing->connect_seq
			     << " == " << connect.connect_seq
			     << dendl;
	  assert(existing->state == STATE_CONNECTING);
	  // make sure our outgoing connection will follow through
	  existing->_send_keepalive();
	  reply.tag = CEPH_MSGR_TAG_WAIT;
	  existing->pipe_lock.Unlock();
	  msgr->lock.Unlock();
	  goto reply;
	}
      }

      assert(connect.connect_seq > existing->connect_seq);
      assert(connect.global_seq >= existing->peer_global_seq);
      if (policy.resetcheck &&   // RESETSESSION only used by servers; peers do not reset each other
	  existing->connect_seq == 0) {
	ldout(msgr->cct,0) << "accept we reset (peer sent cseq " << connect.connect_seq 
		 << ", " << existing << ".cseq = " << existing->connect_seq
		 << "), sending RESETSESSION" << dendl;
	reply.tag = CEPH_MSGR_TAG_RESETSESSION;
	msgr->lock.Unlock();
	existing->pipe_lock.Unlock();
	goto reply;
      }

      // reconnect
      ldout(msgr->cct,10) << "accept peer sent cseq " << connect.connect_seq
	       << " > " << existing->connect_seq << dendl;
      goto replace;
    } // existing
    else if (connect.connect_seq > 0) {
      // we reset, and they are opening a new session
      ldout(msgr->cct,0) << "accept we reset (peer sent cseq " << connect.connect_seq << "), sending RESETSESSION" << dendl;
      msgr->lock.Unlock();
      reply.tag = CEPH_MSGR_TAG_RESETSESSION;
      goto reply;
    } else {
      // new session
      ldout(msgr->cct,10) << "accept new session" << dendl;
      existing = NULL;
      goto open;
    }
    ceph_abort();

  retry_session:
    assert(existing->pipe_lock.is_locked());
    assert(pipe_lock.is_locked());
    reply.tag = CEPH_MSGR_TAG_RETRY_SESSION;
    reply.connect_seq = existing->connect_seq + 1;
    existing->pipe_lock.Unlock();
    msgr->lock.Unlock();
    goto reply;    

  reply:
    assert(pipe_lock.is_locked());
    reply.features = ((uint64_t)connect.features & policy.features_supported) | policy.features_required;
    reply.authorizer_len = authorizer_reply.length();
    pipe_lock.Unlock();
    r = tcp_write((char*)&reply, sizeof(reply));
    if (r < 0)
      goto fail_unlocked;
    if (reply.authorizer_len) {
      r = tcp_write(authorizer_reply.c_str(), authorizer_reply.length());
      if (r < 0)
	goto fail_unlocked;
    }
  }
  
 replace:
  assert(existing->pipe_lock.is_locked());
  assert(pipe_lock.is_locked());
  // if it is a hard reset from peer, we don't need a round-trip to negotiate in/out sequence
  if ((connect.features & CEPH_FEATURE_RECONNECT_SEQ) && !is_reset_from_peer) {
    reply_tag = CEPH_MSGR_TAG_SEQ;
    existing_seq = existing->in_seq;
  }
  ldout(msgr->cct,10) << "accept replacing " << existing << dendl;
  existing->stop();
  existing->unregister_pipe();
  replaced = true;

  if (existing->policy.lossy) {
    // disconnect from the Connection
    assert(existing->connection_state);
    if (existing->connection_state->clear_pipe(existing))
      msgr->dispatch_queue.queue_reset(existing->connection_state.get());
  } else {
    // queue a reset on the new connection, which we're dumping for the old
    msgr->dispatch_queue.queue_reset(connection_state.get());

    // drop my Connection, and take a ref to the existing one. do not
    // clear existing->connection_state, since read_message and
    // write_message both dereference it without pipe_lock.
    connection_state = existing->connection_state;

    // make existing Connection reference us
    connection_state->reset_pipe(this);

    if (existing->delay_thread) {
      existing->delay_thread->steal_for_pipe(this);
      delay_thread = existing->delay_thread;
      existing->delay_thread = NULL;
      delay_thread->flush();
    }

    // steal incoming queue
    uint64_t replaced_conn_id = conn_id;
    conn_id = existing->conn_id;
    existing->conn_id = replaced_conn_id;

    // reset the in_seq if this is a hard reset from peer,
    // otherwise we respect our original connection's value
    in_seq = is_reset_from_peer ? 0 : existing->in_seq;
    in_seq_acked = in_seq;

    // steal outgoing queue and out_seq
    existing->requeue_sent();
    out_seq = existing->out_seq;
    ldout(msgr->cct,10) << "accept re-queuing on out_seq " << out_seq << " in_seq " << in_seq << dendl;
    for (map<int, list<Message*> >::iterator p = existing->out_q.begin();
         p != existing->out_q.end();
         ++p)
      out_q[p->first].splice(out_q[p->first].begin(), p->second);
  }
  existing->stop_and_wait();
  existing->pipe_lock.Unlock();

 open:
  // open
  assert(pipe_lock.is_locked());
  connect_seq = connect.connect_seq + 1;
  peer_global_seq = connect.global_seq;
  assert(state == STATE_ACCEPTING);
  state = STATE_OPEN;
  ldout(msgr->cct,10) << "accept success, connect_seq = " << connect_seq << ", sending READY" << dendl;

  // send READY reply
  reply.tag = (reply_tag ? reply_tag : CEPH_MSGR_TAG_READY);
  reply.features = policy.features_supported;
  reply.global_seq = msgr->get_global_seq();
  reply.connect_seq = connect_seq;
  reply.flags = 0;
  reply.authorizer_len = authorizer_reply.length();
  if (policy.lossy)
    reply.flags = reply.flags | CEPH_MSG_CONNECT_LOSSY;

  connection_state->set_features((uint64_t)reply.features & (uint64_t)connect.features);
  ldout(msgr->cct,10) << "accept features " << connection_state->get_features() << dendl;

  session_security.reset(
      get_auth_session_handler(msgr->cct,
			       connect.authorizer_protocol,
			       session_key,
			       connection_state->get_features()));

  // notify
  msgr->dispatch_queue.queue_accept(connection_state.get());
  msgr->ms_deliver_handle_fast_accept(connection_state.get());

  // ok!
  if (msgr->dispatch_queue.stop)
    goto shutting_down;
  removed = msgr->accepting_pipes.erase(this);
  assert(removed == 1);
  register_pipe();
  msgr->lock.Unlock();
  pipe_lock.Unlock();

  r = tcp_write((char*)&reply, sizeof(reply));
  if (r < 0) {
    goto fail_registered;
  }

  if (reply.authorizer_len) {
    r = tcp_write(authorizer_reply.c_str(), authorizer_reply.length());
    if (r < 0) {
      goto fail_registered;
    }
  }

  if (reply_tag == CEPH_MSGR_TAG_SEQ) {
    if (tcp_write((char*)&existing_seq, sizeof(existing_seq)) < 0) {
      ldout(msgr->cct,2) << "accept write error on in_seq" << dendl;
      goto fail_registered;
    }
    if (tcp_read((char*)&newly_acked_seq, sizeof(newly_acked_seq)) < 0) {
      ldout(msgr->cct,2) << "accept read error on newly_acked_seq" << dendl;
      goto fail_registered;
    }
  }

  pipe_lock.Lock();
  discard_requeued_up_to(newly_acked_seq);
  if (state != STATE_CLOSED) {
    ldout(msgr->cct,10) << "accept starting writer, state " << get_state_name() << dendl;
    start_writer();
  }
  ldout(msgr->cct,20) << "accept done" << dendl;

  maybe_start_delay_thread();

  return 0;   // success.

 fail_registered:
  ldout(msgr->cct, 10) << "accept fault after register" << dendl;

  if (msgr->cct->_conf->ms_inject_internal_delays) {
    ldout(msgr->cct, 10) << " sleep for " << msgr->cct->_conf->ms_inject_internal_delays << dendl;
    utime_t t;
    t.set_from_double(msgr->cct->_conf->ms_inject_internal_delays);
    t.sleep();
  }

 fail_unlocked:
  pipe_lock.Lock();
  if (state != STATE_CLOSED) {
    bool queued = is_queued();
    ldout(msgr->cct, 10) << "  queued = " << (int)queued << dendl;
    if (queued) {
      state = policy.server ? STATE_STANDBY : STATE_CONNECTING;
    } else if (replaced) {
      state = STATE_STANDBY;
    } else {
      state = STATE_CLOSED;
      state_closed = true;
    }
    fault();
    if (queued || replaced)
      start_writer();
  }
  return -1;

 shutting_down:
  msgr->lock.Unlock();
 shutting_down_msgr_unlocked:
  assert(pipe_lock.is_locked());

  if (msgr->cct->_conf->ms_inject_internal_delays) {
    ldout(msgr->cct, 10) << " sleep for " << msgr->cct->_conf->ms_inject_internal_delays << dendl;
    utime_t t;
    t.set_from_double(msgr->cct->_conf->ms_inject_internal_delays);
    t.sleep();
  }

  state = STATE_CLOSED;
  state_closed = true;
  fault();
  return -1;
}