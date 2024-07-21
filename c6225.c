int Pipe::connect()
{
  bool got_bad_auth = false;

  ldout(msgr->cct,10) << "connect " << connect_seq << dendl;
  assert(pipe_lock.is_locked());

  __u32 cseq = connect_seq;
  __u32 gseq = msgr->get_global_seq();

  // stop reader thread
  join_reader();

  pipe_lock.Unlock();
  
  char tag = -1;
  int rc = -1;
  struct msghdr msg;
  struct iovec msgvec[2];
  int msglen;
  char banner[strlen(CEPH_BANNER) + 1];  // extra byte makes coverity happy
  entity_addr_t paddr;
  entity_addr_t peer_addr_for_me, socket_addr;
  AuthAuthorizer *authorizer = NULL;
  bufferlist addrbl, myaddrbl;
  const md_config_t *conf = msgr->cct->_conf;

  // close old socket.  this is safe because we stopped the reader thread above.
  if (sd >= 0)
    ::close(sd);

  // create socket?
  sd = ::socket(peer_addr.get_family(), SOCK_STREAM, 0);
  if (sd < 0) {
    rc = -errno;
    lderr(msgr->cct) << "connect couldn't create socket " << cpp_strerror(rc) << dendl;
    goto fail;
  }

  recv_reset();

  set_socket_options();

  {
    entity_addr_t addr2bind = msgr->get_myaddr();
    if (msgr->cct->_conf->ms_bind_before_connect && (!addr2bind.is_blank_ip())) {
      addr2bind.set_port(0);
      int r = ::bind(sd , addr2bind.get_sockaddr(), addr2bind.get_sockaddr_len());
      if (r < 0) {
        ldout(msgr->cct,2) << "client bind error " << ", " << cpp_strerror(errno) << dendl;
        goto fail;
      }
    }
  }

  // connect!
  ldout(msgr->cct,10) << "connecting to " << peer_addr << dendl;
  rc = ::connect(sd, peer_addr.get_sockaddr(), peer_addr.get_sockaddr_len());
  if (rc < 0) {
    int stored_errno = errno;
    ldout(msgr->cct,2) << "connect error " << peer_addr
	     << ", " << cpp_strerror(stored_errno) << dendl;
    if (stored_errno == ECONNREFUSED) {
      ldout(msgr->cct, 2) << "connection refused!" << dendl;
      msgr->dispatch_queue.queue_refused(connection_state.get());
    }
    goto fail;
  }

  // verify banner
  // FIXME: this should be non-blocking, or in some other way verify the banner as we get it.
  rc = tcp_read((char*)&banner, strlen(CEPH_BANNER));
  if (rc < 0) {
    ldout(msgr->cct,2) << "connect couldn't read banner, " << cpp_strerror(rc) << dendl;
    goto fail;
  }
  if (memcmp(banner, CEPH_BANNER, strlen(CEPH_BANNER))) {
    ldout(msgr->cct,0) << "connect protocol error (bad banner) on peer " << peer_addr << dendl;
    goto fail;
  }

  memset(&msg, 0, sizeof(msg));
  msgvec[0].iov_base = banner;
  msgvec[0].iov_len = strlen(CEPH_BANNER);
  msg.msg_iov = msgvec;
  msg.msg_iovlen = 1;
  msglen = msgvec[0].iov_len;
  rc = do_sendmsg(&msg, msglen);
  if (rc < 0) {
    ldout(msgr->cct,2) << "connect couldn't write my banner, " << cpp_strerror(rc) << dendl;
    goto fail;
  }

  // identify peer
  {
#if defined(__linux__) || defined(DARWIN) || defined(__FreeBSD__)
    bufferptr p(sizeof(ceph_entity_addr) * 2);
#else
    int wirelen = sizeof(__u32) * 2 + sizeof(ceph_sockaddr_storage);
    bufferptr p(wirelen * 2);
#endif
    addrbl.push_back(std::move(p));
  }
  rc = tcp_read(addrbl.c_str(), addrbl.length());
  if (rc < 0) {
    ldout(msgr->cct,2) << "connect couldn't read peer addrs, " << cpp_strerror(rc) << dendl;
    goto fail;
  }
  try {
    bufferlist::iterator p = addrbl.begin();
    ::decode(paddr, p);
    ::decode(peer_addr_for_me, p);
  }
  catch (buffer::error& e) {
    ldout(msgr->cct,2) << "connect couldn't decode peer addrs: " << e.what()
		       << dendl;
    goto fail;
  }
  port = peer_addr_for_me.get_port();

  ldout(msgr->cct,20) << "connect read peer addr " << paddr << " on socket " << sd << dendl;
  if (peer_addr != paddr) {
    if (paddr.is_blank_ip() &&
	peer_addr.get_port() == paddr.get_port() &&
	peer_addr.get_nonce() == paddr.get_nonce()) {
      ldout(msgr->cct,0) << "connect claims to be " 
	      << paddr << " not " << peer_addr << " - presumably this is the same node!" << dendl;
    } else {
      ldout(msgr->cct,10) << "connect claims to be "
			  << paddr << " not " << peer_addr << dendl;
      goto fail;
    }
  }

  ldout(msgr->cct,20) << "connect peer addr for me is " << peer_addr_for_me << dendl;

  msgr->learned_addr(peer_addr_for_me);

  ::encode(msgr->my_inst.addr, myaddrbl, 0);  // legacy

  memset(&msg, 0, sizeof(msg));
  msgvec[0].iov_base = myaddrbl.c_str();
  msgvec[0].iov_len = myaddrbl.length();
  msg.msg_iov = msgvec;
  msg.msg_iovlen = 1;
  msglen = msgvec[0].iov_len;
  rc = do_sendmsg(&msg, msglen);
  if (rc < 0) {
    ldout(msgr->cct,2) << "connect couldn't write my addr, " << cpp_strerror(rc) << dendl;
    goto fail;
  }
  ldout(msgr->cct,10) << "connect sent my addr " << msgr->my_inst.addr << dendl;


  while (1) {
    delete authorizer;
    authorizer = msgr->get_authorizer(peer_type, false);
    bufferlist authorizer_reply;

    ceph_msg_connect connect;
    connect.features = policy.features_supported;
    connect.host_type = msgr->get_myinst().name.type();
    connect.global_seq = gseq;
    connect.connect_seq = cseq;
    connect.protocol_version = msgr->get_proto_version(peer_type, true);
    connect.authorizer_protocol = authorizer ? authorizer->protocol : 0;
    connect.authorizer_len = authorizer ? authorizer->bl.length() : 0;
    if (authorizer) 
      ldout(msgr->cct,10) << "connect.authorizer_len=" << connect.authorizer_len
	       << " protocol=" << connect.authorizer_protocol << dendl;
    connect.flags = 0;
    if (policy.lossy)
      connect.flags |= CEPH_MSG_CONNECT_LOSSY;  // this is fyi, actually, server decides!
    memset(&msg, 0, sizeof(msg));
    msgvec[0].iov_base = (char*)&connect;
    msgvec[0].iov_len = sizeof(connect);
    msg.msg_iov = msgvec;
    msg.msg_iovlen = 1;
    msglen = msgvec[0].iov_len;
    if (authorizer) {
      msgvec[1].iov_base = authorizer->bl.c_str();
      msgvec[1].iov_len = authorizer->bl.length();
      msg.msg_iovlen++;
      msglen += msgvec[1].iov_len;
    }

    ldout(msgr->cct,10) << "connect sending gseq=" << gseq << " cseq=" << cseq
	     << " proto=" << connect.protocol_version << dendl;
    rc = do_sendmsg(&msg, msglen);
    if (rc < 0) {
      ldout(msgr->cct,2) << "connect couldn't write gseq, cseq, " << cpp_strerror(rc) << dendl;
      goto fail;
    }

    ldout(msgr->cct,20) << "connect wrote (self +) cseq, waiting for reply" << dendl;
    ceph_msg_connect_reply reply;
    rc = tcp_read((char*)&reply, sizeof(reply));
    if (rc < 0) {
      ldout(msgr->cct,2) << "connect read reply " << cpp_strerror(rc) << dendl;
      goto fail;
    }

    ldout(msgr->cct,20) << "connect got reply tag " << (int)reply.tag
			<< " connect_seq " << reply.connect_seq
			<< " global_seq " << reply.global_seq
			<< " proto " << reply.protocol_version
			<< " flags " << (int)reply.flags
			<< " features " << reply.features
			<< dendl;

    authorizer_reply.clear();

    if (reply.authorizer_len) {
      ldout(msgr->cct,10) << "reply.authorizer_len=" << reply.authorizer_len << dendl;
      bufferptr bp = buffer::create(reply.authorizer_len);
      rc = tcp_read(bp.c_str(), reply.authorizer_len);
      if (rc < 0) {
        ldout(msgr->cct,10) << "connect couldn't read connect authorizer_reply" << cpp_strerror(rc) << dendl;
	goto fail;
      }
      authorizer_reply.push_back(bp);
    }

    if (authorizer) {
      bufferlist::iterator iter = authorizer_reply.begin();
      if (!authorizer->verify_reply(iter)) {
        ldout(msgr->cct,0) << "failed verifying authorize reply" << dendl;
	goto fail;
      }
    }

    if (conf->ms_inject_internal_delays) {
      ldout(msgr->cct, 10) << " sleep for " << msgr->cct->_conf->ms_inject_internal_delays << dendl;
      utime_t t;
      t.set_from_double(msgr->cct->_conf->ms_inject_internal_delays);
      t.sleep();
    }

    pipe_lock.Lock();
    if (state != STATE_CONNECTING) {
      ldout(msgr->cct,0) << "connect got RESETSESSION but no longer connecting" << dendl;
      goto stop_locked;
    }

    if (reply.tag == CEPH_MSGR_TAG_FEATURES) {
      ldout(msgr->cct,0) << "connect protocol feature mismatch, my " << std::hex
	      << connect.features << " < peer " << reply.features
	      << " missing " << (reply.features & ~policy.features_supported)
	      << std::dec << dendl;
      goto fail_locked;
    }

    if (reply.tag == CEPH_MSGR_TAG_BADPROTOVER) {
      ldout(msgr->cct,0) << "connect protocol version mismatch, my " << connect.protocol_version
	      << " != " << reply.protocol_version << dendl;
      goto fail_locked;
    }

    if (reply.tag == CEPH_MSGR_TAG_BADAUTHORIZER) {
      ldout(msgr->cct,0) << "connect got BADAUTHORIZER" << dendl;
      if (got_bad_auth)
        goto stop_locked;
      got_bad_auth = true;
      pipe_lock.Unlock();
      delete authorizer;
      authorizer = msgr->get_authorizer(peer_type, true);  // try harder
      continue;
    }
    if (reply.tag == CEPH_MSGR_TAG_RESETSESSION) {
      ldout(msgr->cct,0) << "connect got RESETSESSION" << dendl;
      was_session_reset();
      cseq = 0;
      pipe_lock.Unlock();
      continue;
    }
    if (reply.tag == CEPH_MSGR_TAG_RETRY_GLOBAL) {
      gseq = msgr->get_global_seq(reply.global_seq);
      ldout(msgr->cct,10) << "connect got RETRY_GLOBAL " << reply.global_seq
	       << " chose new " << gseq << dendl;
      pipe_lock.Unlock();
      continue;
    }
    if (reply.tag == CEPH_MSGR_TAG_RETRY_SESSION) {
      assert(reply.connect_seq > connect_seq);
      ldout(msgr->cct,10) << "connect got RETRY_SESSION " << connect_seq
	       << " -> " << reply.connect_seq << dendl;
      cseq = connect_seq = reply.connect_seq;
      pipe_lock.Unlock();
      continue;
    }

    if (reply.tag == CEPH_MSGR_TAG_WAIT) {
      ldout(msgr->cct,3) << "connect got WAIT (connection race)" << dendl;
      state = STATE_WAIT;
      goto stop_locked;
    }

    if (reply.tag == CEPH_MSGR_TAG_READY ||
        reply.tag == CEPH_MSGR_TAG_SEQ) {
      uint64_t feat_missing = policy.features_required & ~(uint64_t)reply.features;
      if (feat_missing) {
	ldout(msgr->cct,1) << "missing required features " << std::hex << feat_missing << std::dec << dendl;
	goto fail_locked;
      }

      if (reply.tag == CEPH_MSGR_TAG_SEQ) {
        ldout(msgr->cct,10) << "got CEPH_MSGR_TAG_SEQ, reading acked_seq and writing in_seq" << dendl;
        uint64_t newly_acked_seq = 0;
        rc = tcp_read((char*)&newly_acked_seq, sizeof(newly_acked_seq));
        if (rc < 0) {
          ldout(msgr->cct,2) << "connect read error on newly_acked_seq" << cpp_strerror(rc) << dendl;
          goto fail_locked;
        }
	ldout(msgr->cct,2) << " got newly_acked_seq " << newly_acked_seq
			   << " vs out_seq " << out_seq << dendl;
	while (newly_acked_seq > out_seq) {
	  Message *m = _get_next_outgoing();
	  assert(m);
	  ldout(msgr->cct,2) << " discarding previously sent " << m->get_seq()
			     << " " << *m << dendl;
	  assert(m->get_seq() <= newly_acked_seq);
	  m->put();
	  ++out_seq;
	}
        if (tcp_write((char*)&in_seq, sizeof(in_seq)) < 0) {
          ldout(msgr->cct,2) << "connect write error on in_seq" << dendl;
          goto fail_locked;
        }
      }

      // hooray!
      peer_global_seq = reply.global_seq;
      policy.lossy = reply.flags & CEPH_MSG_CONNECT_LOSSY;
      state = STATE_OPEN;
      connect_seq = cseq + 1;
      assert(connect_seq == reply.connect_seq);
      backoff = utime_t();
      connection_state->set_features((uint64_t)reply.features & (uint64_t)connect.features);
      ldout(msgr->cct,10) << "connect success " << connect_seq << ", lossy = " << policy.lossy
	       << ", features " << connection_state->get_features() << dendl;
      

      // If we have an authorizer, get a new AuthSessionHandler to deal with ongoing security of the
      // connection.  PLR

      if (authorizer != NULL) {
	session_security.reset(
            get_auth_session_handler(msgr->cct,
				     authorizer->protocol,
				     authorizer->session_key,
				     connection_state->get_features()));
      }  else {
        // We have no authorizer, so we shouldn't be applying security to messages in this pipe.  PLR
	session_security.reset();
      }

      msgr->dispatch_queue.queue_connect(connection_state.get());
      msgr->ms_deliver_handle_fast_connect(connection_state.get());
      
      if (!reader_running) {
	ldout(msgr->cct,20) << "connect starting reader" << dendl;
	start_reader();
      }
      maybe_start_delay_thread();
      delete authorizer;
      return 0;
    }
    
    // protocol error
    ldout(msgr->cct,0) << "connect got bad tag " << (int)tag << dendl;
    goto fail_locked;
  }

 fail:
  if (conf->ms_inject_internal_delays) {
    ldout(msgr->cct, 10) << " sleep for " << msgr->cct->_conf->ms_inject_internal_delays << dendl;
    utime_t t;
    t.set_from_double(msgr->cct->_conf->ms_inject_internal_delays);
    t.sleep();
  }

  pipe_lock.Lock();
 fail_locked:
  if (state == STATE_CONNECTING)
    fault();
  else
    ldout(msgr->cct,3) << "connect fault, but state = " << get_state_name()
		       << " != connecting, stopping" << dendl;

 stop_locked:
  delete authorizer;
  return rc;
}