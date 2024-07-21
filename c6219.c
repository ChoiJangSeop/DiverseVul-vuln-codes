ssize_t AsyncConnection::_process_connection()
{
  ssize_t r = 0;

  switch(state) {
    case STATE_WAIT_SEND:
      {
        std::lock_guard<std::mutex> l(write_lock);
        if (!outcoming_bl.length()) {
          assert(state_after_send);
          state = state_after_send;
          state_after_send = STATE_NONE;
        }
        break;
      }

    case STATE_CONNECTING:
      {
        assert(!policy.server);

        // reset connect state variables
        got_bad_auth = false;
        delete authorizer;
        authorizer = NULL;
        authorizer_buf.clear();
        memset(&connect_msg, 0, sizeof(connect_msg));
        memset(&connect_reply, 0, sizeof(connect_reply));

        global_seq = async_msgr->get_global_seq();
        // close old socket.  this is safe because we stopped the reader thread above.
        if (cs) {
          center->delete_file_event(cs.fd(), EVENT_READABLE|EVENT_WRITABLE);
          cs.close();
        }

        SocketOptions opts;
        opts.priority = async_msgr->get_socket_priority();
        opts.connect_bind_addr = msgr->get_myaddr();
        r = worker->connect(get_peer_addr(), opts, &cs);
        if (r < 0)
          goto fail;

        center->create_file_event(cs.fd(), EVENT_READABLE, read_handler);
        state = STATE_CONNECTING_RE;
        break;
      }

    case STATE_CONNECTING_RE:
      {
        r = cs.is_connected();
        if (r < 0) {
          ldout(async_msgr->cct, 1) << __func__ << " reconnect failed " << dendl;
          if (r == -ECONNREFUSED) {
            ldout(async_msgr->cct, 2) << __func__ << " connection refused!" << dendl;
            dispatch_queue->queue_refused(this);
          }
          goto fail;
        } else if (r == 0) {
          ldout(async_msgr->cct, 10) << __func__ << " nonblock connect inprogress" << dendl;
          if (async_msgr->get_stack()->nonblock_connect_need_writable_event())
            center->create_file_event(cs.fd(), EVENT_WRITABLE, read_handler);
          break;
        }

        center->delete_file_event(cs.fd(), EVENT_WRITABLE);
        ldout(async_msgr->cct, 10) << __func__ << " connect successfully, ready to send banner" << dendl;

        bufferlist bl;
        bl.append(CEPH_BANNER, strlen(CEPH_BANNER));
        r = try_send(bl);
        if (r == 0) {
          state = STATE_CONNECTING_WAIT_BANNER_AND_IDENTIFY;
          ldout(async_msgr->cct, 10) << __func__ << " connect write banner done: "
                                     << get_peer_addr() << dendl;
        } else if (r > 0) {
          state = STATE_WAIT_SEND;
          state_after_send = STATE_CONNECTING_WAIT_BANNER_AND_IDENTIFY;
          ldout(async_msgr->cct, 10) << __func__ << " connect wait for write banner: "
                               << get_peer_addr() << dendl;
        } else {
          goto fail;
        }

        break;
      }

    case STATE_CONNECTING_WAIT_BANNER_AND_IDENTIFY:
      {
        entity_addr_t paddr, peer_addr_for_me;
        bufferlist myaddrbl;
        unsigned banner_len = strlen(CEPH_BANNER);
        unsigned need_len = banner_len + sizeof(ceph_entity_addr)*2;
        r = read_until(need_len, state_buffer);
        if (r < 0) {
          ldout(async_msgr->cct, 1) << __func__ << " read banner and identify addresses failed" << dendl;
          goto fail;
        } else if (r > 0) {
          break;
        }

        if (memcmp(state_buffer, CEPH_BANNER, banner_len)) {
          ldout(async_msgr->cct, 0) << __func__ << " connect protocol error (bad banner) on peer "
                                    << get_peer_addr() << dendl;
          goto fail;
        }

        bufferlist bl;
        bl.append(state_buffer+banner_len, sizeof(ceph_entity_addr)*2);
        bufferlist::iterator p = bl.begin();
        try {
          ::decode(paddr, p);
          ::decode(peer_addr_for_me, p);
        } catch (const buffer::error& e) {
          lderr(async_msgr->cct) << __func__ <<  " decode peer addr failed " << dendl;
          goto fail;
        }
        ldout(async_msgr->cct, 20) << __func__ <<  " connect read peer addr "
                             << paddr << " on socket " << cs.fd() << dendl;
        if (peer_addr != paddr) {
          if (paddr.is_blank_ip() && peer_addr.get_port() == paddr.get_port() &&
              peer_addr.get_nonce() == paddr.get_nonce()) {
            ldout(async_msgr->cct, 0) << __func__ <<  " connect claims to be " << paddr
                                << " not " << peer_addr
                                << " - presumably this is the same node!" << dendl;
          } else {
            ldout(async_msgr->cct, 10) << __func__ << " connect claims to be "
				       << paddr << " not " << peer_addr << dendl;
	    goto fail;
          }
        }

        ldout(async_msgr->cct, 20) << __func__ << " connect peer addr for me is " << peer_addr_for_me << dendl;
        lock.unlock();
        async_msgr->learned_addr(peer_addr_for_me);
        if (async_msgr->cct->_conf->ms_inject_internal_delays
            && async_msgr->cct->_conf->ms_inject_socket_failures) {
          if (rand() % async_msgr->cct->_conf->ms_inject_socket_failures == 0) {
            ldout(msgr->cct, 10) << __func__ << " sleep for "
                                 << async_msgr->cct->_conf->ms_inject_internal_delays << dendl;
            utime_t t;
            t.set_from_double(async_msgr->cct->_conf->ms_inject_internal_delays);
            t.sleep();
          }
        }

        lock.lock();
        if (state != STATE_CONNECTING_WAIT_BANNER_AND_IDENTIFY) {
          ldout(async_msgr->cct, 1) << __func__ << " state changed while learned_addr, mark_down or "
                                    << " replacing must be happened just now" << dendl;
          return 0;
        }

        ::encode(async_msgr->get_myaddr(), myaddrbl, 0); // legacy
        r = try_send(myaddrbl);
        if (r == 0) {
          state = STATE_CONNECTING_SEND_CONNECT_MSG;
          ldout(async_msgr->cct, 10) << __func__ << " connect sent my addr "
              << async_msgr->get_myaddr() << dendl;
        } else if (r > 0) {
          state = STATE_WAIT_SEND;
          state_after_send = STATE_CONNECTING_SEND_CONNECT_MSG;
          ldout(async_msgr->cct, 10) << __func__ << " connect send my addr done: "
              << async_msgr->get_myaddr() << dendl;
        } else {
          ldout(async_msgr->cct, 2) << __func__ << " connect couldn't write my addr, "
              << cpp_strerror(r) << dendl;
          goto fail;
        }

        break;
      }

    case STATE_CONNECTING_SEND_CONNECT_MSG:
      {
        if (!got_bad_auth) {
          delete authorizer;
          authorizer = async_msgr->get_authorizer(peer_type, false);
        }
        bufferlist bl;

        connect_msg.features = policy.features_supported;
        connect_msg.host_type = async_msgr->get_myinst().name.type();
        connect_msg.global_seq = global_seq;
        connect_msg.connect_seq = connect_seq;
        connect_msg.protocol_version = async_msgr->get_proto_version(peer_type, true);
        connect_msg.authorizer_protocol = authorizer ? authorizer->protocol : 0;
        connect_msg.authorizer_len = authorizer ? authorizer->bl.length() : 0;
        if (authorizer)
          ldout(async_msgr->cct, 10) << __func__ <<  " connect_msg.authorizer_len="
                                     << connect_msg.authorizer_len << " protocol="
                                     << connect_msg.authorizer_protocol << dendl;
        connect_msg.flags = 0;
        if (policy.lossy)
          connect_msg.flags |= CEPH_MSG_CONNECT_LOSSY;  // this is fyi, actually, server decides!
        bl.append((char*)&connect_msg, sizeof(connect_msg));
        if (authorizer) {
          bl.append(authorizer->bl.c_str(), authorizer->bl.length());
        }
        ldout(async_msgr->cct, 10) << __func__ << " connect sending gseq=" << global_seq << " cseq="
            << connect_seq << " proto=" << connect_msg.protocol_version << dendl;

        r = try_send(bl);
        if (r == 0) {
          state = STATE_CONNECTING_WAIT_CONNECT_REPLY;
          ldout(async_msgr->cct,20) << __func__ << " connect wrote (self +) cseq, waiting for reply" << dendl;
        } else if (r > 0) {
          state = STATE_WAIT_SEND;
          state_after_send = STATE_CONNECTING_WAIT_CONNECT_REPLY;
          ldout(async_msgr->cct, 10) << __func__ << " continue send reply " << dendl;
        } else {
          ldout(async_msgr->cct, 2) << __func__ << " connect couldn't send reply "
              << cpp_strerror(r) << dendl;
          goto fail;
        }

        break;
      }

    case STATE_CONNECTING_WAIT_CONNECT_REPLY:
      {
        r = read_until(sizeof(connect_reply), state_buffer);
        if (r < 0) {
          ldout(async_msgr->cct, 1) << __func__ << " read connect reply failed" << dendl;
          goto fail;
        } else if (r > 0) {
          break;
        }

        connect_reply = *((ceph_msg_connect_reply*)state_buffer);

        ldout(async_msgr->cct, 20) << __func__ << " connect got reply tag " << (int)connect_reply.tag
                             << " connect_seq " << connect_reply.connect_seq << " global_seq "
                             << connect_reply.global_seq << " proto " << connect_reply.protocol_version
                             << " flags " << (int)connect_reply.flags << " features "
                             << connect_reply.features << dendl;
        state = STATE_CONNECTING_WAIT_CONNECT_REPLY_AUTH;

        break;
      }

    case STATE_CONNECTING_WAIT_CONNECT_REPLY_AUTH:
      {
        bufferlist authorizer_reply;
        if (connect_reply.authorizer_len) {
          ldout(async_msgr->cct, 10) << __func__ << " reply.authorizer_len=" << connect_reply.authorizer_len << dendl;
          assert(connect_reply.authorizer_len < 4096);
          r = read_until(connect_reply.authorizer_len, state_buffer);
          if (r < 0) {
            ldout(async_msgr->cct, 1) << __func__ << " read connect reply authorizer failed" << dendl;
            goto fail;
          } else if (r > 0) {
            break;
          }

          authorizer_reply.append(state_buffer, connect_reply.authorizer_len);
          bufferlist::iterator iter = authorizer_reply.begin();
          if (authorizer && !authorizer->verify_reply(iter)) {
            ldout(async_msgr->cct, 0) << __func__ << " failed verifying authorize reply" << dendl;
            goto fail;
          }
        }
        r = handle_connect_reply(connect_msg, connect_reply);
        if (r < 0)
          goto fail;

        // state must be changed!
        assert(state != STATE_CONNECTING_WAIT_CONNECT_REPLY_AUTH);
        break;
      }

    case STATE_CONNECTING_WAIT_ACK_SEQ:
      {
        uint64_t newly_acked_seq = 0;

        r = read_until(sizeof(newly_acked_seq), state_buffer);
        if (r < 0) {
          ldout(async_msgr->cct, 1) << __func__ << " read connect ack seq failed" << dendl;
          goto fail;
        } else if (r > 0) {
          break;
        }

        newly_acked_seq = *((uint64_t*)state_buffer);
        ldout(async_msgr->cct, 2) << __func__ << " got newly_acked_seq " << newly_acked_seq
                            << " vs out_seq " << out_seq << dendl;
        discard_requeued_up_to(newly_acked_seq);
        //while (newly_acked_seq > out_seq.read()) {
        //  Message *m = _get_next_outgoing(NULL);
        //  assert(m);
        //  ldout(async_msgr->cct, 2) << __func__ << " discarding previously sent " << m->get_seq()
        //                      << " " << *m << dendl;
        //  assert(m->get_seq() <= newly_acked_seq);
        //  m->put();
        //  out_seq.inc();
        //}

        bufferlist bl;
        uint64_t s = in_seq;
        bl.append((char*)&s, sizeof(s));
        r = try_send(bl);
        if (r == 0) {
          state = STATE_CONNECTING_READY;
          ldout(async_msgr->cct, 10) << __func__ << " send in_seq done " << dendl;
        } else if (r > 0) {
          state_after_send = STATE_CONNECTING_READY;
          state = STATE_WAIT_SEND;
          ldout(async_msgr->cct, 10) << __func__ << " continue send in_seq " << dendl;
        } else {
          goto fail;
        }
        break;
      }

    case STATE_CONNECTING_READY:
      {
        // hooray!
        peer_global_seq = connect_reply.global_seq;
        policy.lossy = connect_reply.flags & CEPH_MSG_CONNECT_LOSSY;
        state = STATE_OPEN;
        once_ready = true;
        connect_seq += 1;
        assert(connect_seq == connect_reply.connect_seq);
        backoff = utime_t();
        set_features((uint64_t)connect_reply.features & (uint64_t)connect_msg.features);
        ldout(async_msgr->cct, 10) << __func__ << " connect success " << connect_seq
                                   << ", lossy = " << policy.lossy << ", features "
                                   << get_features() << dendl;

        // If we have an authorizer, get a new AuthSessionHandler to deal with ongoing security of the
        // connection.  PLR
        if (authorizer != NULL) {
          session_security.reset(
              get_auth_session_handler(async_msgr->cct,
                                       authorizer->protocol,
                                       authorizer->session_key,
                                       get_features()));
        } else {
          // We have no authorizer, so we shouldn't be applying security to messages in this AsyncConnection.  PLR
          session_security.reset();
        }

        if (delay_state)
          assert(delay_state->ready());
        dispatch_queue->queue_connect(this);
        async_msgr->ms_deliver_handle_fast_connect(this);

        // make sure no pending tick timer
        if (last_tick_id)
          center->delete_time_event(last_tick_id);
        last_tick_id = center->create_time_event(inactive_timeout_us, tick_handler);

        // message may in queue between last _try_send and connection ready
        // write event may already notify and we need to force scheduler again
        write_lock.lock();
        can_write = WriteStatus::CANWRITE;
        if (is_queued())
          center->dispatch_event_external(write_handler);
        write_lock.unlock();
        maybe_start_delay_thread();
        break;
      }

    case STATE_ACCEPTING:
      {
        bufferlist bl;
        center->create_file_event(cs.fd(), EVENT_READABLE, read_handler);

        bl.append(CEPH_BANNER, strlen(CEPH_BANNER));

        ::encode(async_msgr->get_myaddr(), bl, 0); // legacy
        port = async_msgr->get_myaddr().get_port();
        ::encode(socket_addr, bl, 0); // legacy
        ldout(async_msgr->cct, 1) << __func__ << " sd=" << cs.fd() << " " << socket_addr << dendl;

        r = try_send(bl);
        if (r == 0) {
          state = STATE_ACCEPTING_WAIT_BANNER_ADDR;
          ldout(async_msgr->cct, 10) << __func__ << " write banner and addr done: "
            << get_peer_addr() << dendl;
        } else if (r > 0) {
          state = STATE_WAIT_SEND;
          state_after_send = STATE_ACCEPTING_WAIT_BANNER_ADDR;
          ldout(async_msgr->cct, 10) << __func__ << " wait for write banner and addr: "
                              << get_peer_addr() << dendl;
        } else {
          goto fail;
        }

        break;
      }
    case STATE_ACCEPTING_WAIT_BANNER_ADDR:
      {
        bufferlist addr_bl;
        entity_addr_t peer_addr;

        r = read_until(strlen(CEPH_BANNER) + sizeof(ceph_entity_addr), state_buffer);
        if (r < 0) {
          ldout(async_msgr->cct, 1) << __func__ << " read peer banner and addr failed" << dendl;
          goto fail;
        } else if (r > 0) {
          break;
        }

        if (memcmp(state_buffer, CEPH_BANNER, strlen(CEPH_BANNER))) {
          ldout(async_msgr->cct, 1) << __func__ << " accept peer sent bad banner '" << state_buffer
                                    << "' (should be '" << CEPH_BANNER << "')" << dendl;
          goto fail;
        }

        addr_bl.append(state_buffer+strlen(CEPH_BANNER), sizeof(ceph_entity_addr));
        {
          bufferlist::iterator ti = addr_bl.begin();
          ::decode(peer_addr, ti);
        }

        ldout(async_msgr->cct, 10) << __func__ << " accept peer addr is " << peer_addr << dendl;
        if (peer_addr.is_blank_ip()) {
          // peer apparently doesn't know what ip they have; figure it out for them.
          int port = peer_addr.get_port();
          peer_addr.u = socket_addr.u;
          peer_addr.set_port(port);
          ldout(async_msgr->cct, 0) << __func__ << " accept peer addr is really " << peer_addr
                             << " (socket is " << socket_addr << ")" << dendl;
        }
        set_peer_addr(peer_addr);  // so that connection_state gets set up
        state = STATE_ACCEPTING_WAIT_CONNECT_MSG;
        break;
      }

    case STATE_ACCEPTING_WAIT_CONNECT_MSG:
      {
        r = read_until(sizeof(connect_msg), state_buffer);
        if (r < 0) {
          ldout(async_msgr->cct, 1) << __func__ << " read connect msg failed" << dendl;
          goto fail;
        } else if (r > 0) {
          break;
        }

        connect_msg = *((ceph_msg_connect*)state_buffer);
        state = STATE_ACCEPTING_WAIT_CONNECT_MSG_AUTH;
        break;
      }

    case STATE_ACCEPTING_WAIT_CONNECT_MSG_AUTH:
      {
        bufferlist authorizer_reply;

        if (connect_msg.authorizer_len) {
          if (!authorizer_buf.length())
            authorizer_buf.push_back(buffer::create(connect_msg.authorizer_len));

          r = read_until(connect_msg.authorizer_len, authorizer_buf.c_str());
          if (r < 0) {
            ldout(async_msgr->cct, 1) << __func__ << " read connect authorizer failed" << dendl;
            goto fail;
          } else if (r > 0) {
            break;
          }
        }

        ldout(async_msgr->cct, 20) << __func__ << " accept got peer connect_seq "
                             << connect_msg.connect_seq << " global_seq "
                             << connect_msg.global_seq << dendl;
        set_peer_type(connect_msg.host_type);
        policy = async_msgr->get_policy(connect_msg.host_type);
        ldout(async_msgr->cct, 10) << __func__ << " accept of host_type " << connect_msg.host_type
                                   << ", policy.lossy=" << policy.lossy << " policy.server="
                                   << policy.server << " policy.standby=" << policy.standby
                                   << " policy.resetcheck=" << policy.resetcheck << dendl;

        r = handle_connect_msg(connect_msg, authorizer_buf, authorizer_reply);
        if (r < 0)
          goto fail;

        // state is changed by "handle_connect_msg"
        assert(state != STATE_ACCEPTING_WAIT_CONNECT_MSG_AUTH);
        break;
      }

    case STATE_ACCEPTING_WAIT_SEQ:
      {
        uint64_t newly_acked_seq;
        r = read_until(sizeof(newly_acked_seq), state_buffer);
        if (r < 0) {
          ldout(async_msgr->cct, 1) << __func__ << " read ack seq failed" << dendl;
          goto fail_registered;
        } else if (r > 0) {
          break;
        }

        newly_acked_seq = *((uint64_t*)state_buffer);
        ldout(async_msgr->cct, 2) << __func__ << " accept get newly_acked_seq " << newly_acked_seq << dendl;
        discard_requeued_up_to(newly_acked_seq);
        state = STATE_ACCEPTING_READY;
        break;
      }

    case STATE_ACCEPTING_READY:
      {
        ldout(async_msgr->cct, 20) << __func__ << " accept done" << dendl;
        state = STATE_OPEN;
        memset(&connect_msg, 0, sizeof(connect_msg));

        if (delay_state)
          assert(delay_state->ready());
        // make sure no pending tick timer
        if (last_tick_id)
          center->delete_time_event(last_tick_id);
        last_tick_id = center->create_time_event(inactive_timeout_us, tick_handler);

        write_lock.lock();
        can_write = WriteStatus::CANWRITE;
        if (is_queued())
          center->dispatch_event_external(write_handler);
        write_lock.unlock();
        maybe_start_delay_thread();
        break;
      }

    default:
      {
        lderr(async_msgr->cct) << __func__ << " bad state: " << state << dendl;
        ceph_abort();
      }
  }

  return 0;

fail_registered:
  ldout(async_msgr->cct, 10) << "accept fault after register" << dendl;
  inject_delay();

fail:
  return -1;
}