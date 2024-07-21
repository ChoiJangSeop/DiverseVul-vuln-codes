ssize_t AsyncConnection::handle_connect_msg(ceph_msg_connect &connect, bufferlist &authorizer_bl,
                                            bufferlist &authorizer_reply)
{
  ssize_t r = 0;
  ceph_msg_connect_reply reply;
  bufferlist reply_bl;

  memset(&reply, 0, sizeof(reply));
  reply.protocol_version = async_msgr->get_proto_version(peer_type, false);

  // mismatch?
  ldout(async_msgr->cct, 10) << __func__ << " accept my proto " << reply.protocol_version
                      << ", their proto " << connect.protocol_version << dendl;
  if (connect.protocol_version != reply.protocol_version) {
    return _reply_accept(CEPH_MSGR_TAG_BADPROTOVER, connect, reply, authorizer_reply);
  }
  // require signatures for cephx?
  if (connect.authorizer_protocol == CEPH_AUTH_CEPHX) {
    if (peer_type == CEPH_ENTITY_TYPE_OSD ||
        peer_type == CEPH_ENTITY_TYPE_MDS ||
	peer_type == CEPH_ENTITY_TYPE_MGR) {
      if (async_msgr->cct->_conf->cephx_require_signatures ||
          async_msgr->cct->_conf->cephx_cluster_require_signatures) {
        ldout(async_msgr->cct, 10) << __func__ << " using cephx, requiring MSG_AUTH feature bit for cluster" << dendl;
        policy.features_required |= CEPH_FEATURE_MSG_AUTH;
      }
      if (async_msgr->cct->_conf->cephx_require_version >= 2 ||
	  async_msgr->cct->_conf->cephx_cluster_require_version >= 2) {
        ldout(async_msgr->cct, 10) << __func__ << " using cephx, requiring cephx v2 feature bit for cluster" << dendl;
        policy.features_required |= CEPH_FEATUREMASK_CEPHX_V2;
      }
    } else {
      if (async_msgr->cct->_conf->cephx_require_signatures ||
          async_msgr->cct->_conf->cephx_service_require_signatures) {
        ldout(async_msgr->cct, 10) << __func__ << " using cephx, requiring MSG_AUTH feature bit for service" << dendl;
        policy.features_required |= CEPH_FEATURE_MSG_AUTH;
      }
      if (async_msgr->cct->_conf->cephx_require_version >= 2 ||
	  async_msgr->cct->_conf->cephx_service_require_version >= 2) {
        ldout(async_msgr->cct, 10) << __func__ << " using cephx, requiring cephx v2 feature bit for service" << dendl;
        policy.features_required |= CEPH_FEATUREMASK_CEPHX_V2;
      }
    }
  }

  uint64_t feat_missing = policy.features_required & ~(uint64_t)connect.features;
  if (feat_missing) {
    ldout(async_msgr->cct, 1) << __func__ << " peer missing required features "
                        << std::hex << feat_missing << std::dec << dendl;
    return _reply_accept(CEPH_MSGR_TAG_FEATURES, connect, reply, authorizer_reply);
  }

  lock.unlock();

  bool authorizer_valid;
  if (!async_msgr->verify_authorizer(this, peer_type, connect.authorizer_protocol, authorizer_bl,
                               authorizer_reply, authorizer_valid, session_key) || !authorizer_valid) {
    lock.lock();
    ldout(async_msgr->cct,0) << __func__ << ": got bad authorizer" << dendl;
    session_security.reset();
    return _reply_accept(CEPH_MSGR_TAG_BADAUTHORIZER, connect, reply, authorizer_reply);
  }

  // We've verified the authorizer for this AsyncConnection, so set up the session security structure.  PLR
  ldout(async_msgr->cct, 10) << __func__ << " accept setting up session_security." << dendl;

  // existing?
  AsyncConnectionRef existing = async_msgr->lookup_conn(peer_addr);

  inject_delay();

  lock.lock();
  if (state != STATE_ACCEPTING_WAIT_CONNECT_MSG_AUTH) {
    ldout(async_msgr->cct, 1) << __func__ << " state changed while accept, it must be mark_down" << dendl;
    assert(state == STATE_CLOSED);
    goto fail;
  }

  if (existing == this)
    existing = NULL;
  if (existing) {
    // There is no possible that existing connection will acquire this
    // connection's lock
    existing->lock.lock();  // skip lockdep check (we are locking a second AsyncConnection here)

    if (existing->state == STATE_CLOSED) {
      ldout(async_msgr->cct, 1) << __func__ << " existing already closed." << dendl;
      existing->lock.unlock();
      existing = NULL;
      goto open;
    }

    if (existing->replacing) {
      ldout(async_msgr->cct, 1) << __func__ << " existing racing replace happened while replacing."
                                << " existing_state=" << get_state_name(existing->state) << dendl;
      reply.global_seq = existing->peer_global_seq;
      r = _reply_accept(CEPH_MSGR_TAG_RETRY_GLOBAL, connect, reply, authorizer_reply);
      existing->lock.unlock();
      if (r < 0)
        goto fail;
      return 0;
    }

    if (connect.global_seq < existing->peer_global_seq) {
      ldout(async_msgr->cct, 10) << __func__ << " accept existing " << existing
                           << ".gseq " << existing->peer_global_seq << " > "
                           << connect.global_seq << ", RETRY_GLOBAL" << dendl;
      reply.global_seq = existing->peer_global_seq;  // so we can send it below..
      existing->lock.unlock();
      return _reply_accept(CEPH_MSGR_TAG_RETRY_GLOBAL, connect, reply, authorizer_reply);
    } else {
      ldout(async_msgr->cct, 10) << __func__ << " accept existing " << existing
                           << ".gseq " << existing->peer_global_seq
                           << " <= " << connect.global_seq << ", looks ok" << dendl;
    }

    if (existing->policy.lossy) {
      ldout(async_msgr->cct, 0) << __func__ << " accept replacing existing (lossy) channel (new one lossy="
                          << policy.lossy << ")" << dendl;
      existing->was_session_reset();
      goto replace;
    }

    ldout(async_msgr->cct, 0) << __func__ << " accept connect_seq " << connect.connect_seq
                              << " vs existing csq=" << existing->connect_seq << " existing_state="
                              << get_state_name(existing->state) << dendl;

    if (connect.connect_seq == 0 && existing->connect_seq > 0) {
      ldout(async_msgr->cct,0) << __func__ << " accept peer reset, then tried to connect to us, replacing" << dendl;
      // this is a hard reset from peer
      is_reset_from_peer = true;
      if (policy.resetcheck)
        existing->was_session_reset(); // this resets out_queue, msg_ and connect_seq #'s
      goto replace;
    }

    if (connect.connect_seq < existing->connect_seq) {
      // old attempt, or we sent READY but they didn't get it.
      ldout(async_msgr->cct, 10) << __func__ << " accept existing " << existing << ".cseq "
                           << existing->connect_seq << " > " << connect.connect_seq
                           << ", RETRY_SESSION" << dendl;
      reply.connect_seq = existing->connect_seq + 1;
      existing->lock.unlock();
      return _reply_accept(CEPH_MSGR_TAG_RETRY_SESSION, connect, reply, authorizer_reply);
    }

    if (connect.connect_seq == existing->connect_seq) {
      // if the existing connection successfully opened, and/or
      // subsequently went to standby, then the peer should bump
      // their connect_seq and retry: this is not a connection race
      // we need to resolve here.
      if (existing->state == STATE_OPEN ||
          existing->state == STATE_STANDBY) {
        ldout(async_msgr->cct, 10) << __func__ << " accept connection race, existing " << existing
                             << ".cseq " << existing->connect_seq << " == "
                             << connect.connect_seq << ", OPEN|STANDBY, RETRY_SESSION" << dendl;
        reply.connect_seq = existing->connect_seq + 1;
        existing->lock.unlock();
        return _reply_accept(CEPH_MSGR_TAG_RETRY_SESSION, connect, reply, authorizer_reply);
      }

      // connection race?
      if (peer_addr < async_msgr->get_myaddr() || existing->policy.server) {
        // incoming wins
        ldout(async_msgr->cct, 10) << __func__ << " accept connection race, existing " << existing
                             << ".cseq " << existing->connect_seq << " == " << connect.connect_seq
                             << ", or we are server, replacing my attempt" << dendl;
        goto replace;
      } else {
        // our existing outgoing wins
        ldout(async_msgr->cct,10) << __func__ << " accept connection race, existing "
                            << existing << ".cseq " << existing->connect_seq
                            << " == " << connect.connect_seq << ", sending WAIT" << dendl;
        assert(peer_addr > async_msgr->get_myaddr());
        existing->lock.unlock();
        return _reply_accept(CEPH_MSGR_TAG_WAIT, connect, reply, authorizer_reply);
      }
    }

    assert(connect.connect_seq > existing->connect_seq);
    assert(connect.global_seq >= existing->peer_global_seq);
    if (policy.resetcheck &&   // RESETSESSION only used by servers; peers do not reset each other
        existing->connect_seq == 0) {
      ldout(async_msgr->cct, 0) << __func__ << " accept we reset (peer sent cseq "
                          << connect.connect_seq << ", " << existing << ".cseq = "
                          << existing->connect_seq << "), sending RESETSESSION" << dendl;
      existing->lock.unlock();
      return _reply_accept(CEPH_MSGR_TAG_RESETSESSION, connect, reply, authorizer_reply);
    }

    // reconnect
    ldout(async_msgr->cct, 10) << __func__ << " accept peer sent cseq " << connect.connect_seq
                         << " > " << existing->connect_seq << dendl;
    goto replace;
  } // existing
  else if (!replacing && connect.connect_seq > 0) {
    // we reset, and they are opening a new session
    ldout(async_msgr->cct, 0) << __func__ << " accept we reset (peer sent cseq "
                        << connect.connect_seq << "), sending RESETSESSION" << dendl;
    return _reply_accept(CEPH_MSGR_TAG_RESETSESSION, connect, reply, authorizer_reply);
  } else {
    // new session
    ldout(async_msgr->cct, 10) << __func__ << " accept new session" << dendl;
    existing = NULL;
    goto open;
  }
  ceph_abort();

 replace:
  ldout(async_msgr->cct, 10) << __func__ << " accept replacing " << existing << dendl;

  inject_delay();
  if (existing->policy.lossy) {
    // disconnect from the Connection
    ldout(async_msgr->cct, 1) << __func__ << " replacing on lossy channel, failing existing" << dendl;
    existing->_stop();
    existing->dispatch_queue->queue_reset(existing.get());
  } else {
    assert(can_write == WriteStatus::NOWRITE);
    existing->write_lock.lock();

    // reset the in_seq if this is a hard reset from peer,
    // otherwise we respect our original connection's value
    if (is_reset_from_peer) {
      existing->is_reset_from_peer = true;
    }

    center->delete_file_event(cs.fd(), EVENT_READABLE|EVENT_WRITABLE);

    if (existing->delay_state) {
      existing->delay_state->flush();
      assert(!delay_state);
    }
    existing->reset_recv_state();

    auto temp_cs = std::move(cs);
    EventCenter *new_center = center;
    Worker *new_worker = worker;
    // avoid _stop shutdown replacing socket
    // queue a reset on the new connection, which we're dumping for the old
    _stop();

    dispatch_queue->queue_reset(this);
    ldout(async_msgr->cct, 1) << __func__ << " stop myself to swap existing" << dendl;
    existing->can_write = WriteStatus::REPLACING;
    existing->replacing = true;
    existing->state_offset = 0;
    // avoid previous thread modify event
    existing->state = STATE_NONE;
    // Discard existing prefetch buffer in `recv_buf`
    existing->recv_start = existing->recv_end = 0;
    // there shouldn't exist any buffer
    assert(recv_start == recv_end);

    auto deactivate_existing = std::bind(
        [existing, new_worker, new_center, connect, reply, authorizer_reply](ConnectedSocket &cs) mutable {
      // we need to delete time event in original thread
      {
        std::lock_guard<std::mutex> l(existing->lock);
        existing->write_lock.lock();
        existing->requeue_sent();
        existing->outcoming_bl.clear();
        existing->open_write = false;
        existing->write_lock.unlock();
        if (existing->state == STATE_NONE) {
          existing->shutdown_socket();
          existing->cs = std::move(cs);
          existing->worker->references--;
          new_worker->references++;
          existing->logger = new_worker->get_perf_counter();
          existing->worker = new_worker;
          existing->center = new_center;
          if (existing->delay_state)
            existing->delay_state->set_center(new_center);
        } else if (existing->state == STATE_CLOSED) {
          auto back_to_close = std::bind(
            [](ConnectedSocket &cs) mutable { cs.close(); }, std::move(cs));
          new_center->submit_to(
              new_center->get_id(), std::move(back_to_close), true);
          return ;
        } else {
          ceph_abort();
        }
      }

      // Before changing existing->center, it may already exists some events in existing->center's queue.
      // Then if we mark down `existing`, it will execute in another thread and clean up connection.
      // Previous event will result in segment fault
      auto transfer_existing = [existing, connect, reply, authorizer_reply]() mutable {
        std::lock_guard<std::mutex> l(existing->lock);
        if (existing->state == STATE_CLOSED)
          return ;
        assert(existing->state == STATE_NONE);
  
        existing->state = STATE_ACCEPTING_WAIT_CONNECT_MSG;
        existing->center->create_file_event(existing->cs.fd(), EVENT_READABLE, existing->read_handler);
        reply.global_seq = existing->peer_global_seq;
        if (existing->_reply_accept(CEPH_MSGR_TAG_RETRY_GLOBAL, connect, reply, authorizer_reply) < 0) {
          // handle error
          existing->fault();
        }
      };
      if (existing->center->in_thread())
        transfer_existing();
      else
        existing->center->submit_to(
            existing->center->get_id(), std::move(transfer_existing), true);
    }, std::move(temp_cs));

    existing->center->submit_to(
        existing->center->get_id(), std::move(deactivate_existing), true);
    existing->write_lock.unlock();
    existing->lock.unlock();
    return 0;
  }
  existing->lock.unlock();

 open:
  connect_seq = connect.connect_seq + 1;
  peer_global_seq = connect.global_seq;
  ldout(async_msgr->cct, 10) << __func__ << " accept success, connect_seq = "
                             << connect_seq << " in_seq=" << in_seq << ", sending READY" << dendl;

  int next_state;

  // if it is a hard reset from peer, we don't need a round-trip to negotiate in/out sequence
  if ((connect.features & CEPH_FEATURE_RECONNECT_SEQ) && !is_reset_from_peer) {
    reply.tag = CEPH_MSGR_TAG_SEQ;
    next_state = STATE_ACCEPTING_WAIT_SEQ;
  } else {
    reply.tag = CEPH_MSGR_TAG_READY;
    next_state = STATE_ACCEPTING_READY;
    discard_requeued_up_to(0);
    is_reset_from_peer = false;
    in_seq = 0;
  }

  // send READY reply
  reply.features = policy.features_supported;
  reply.global_seq = async_msgr->get_global_seq();
  reply.connect_seq = connect_seq;
  reply.flags = 0;
  reply.authorizer_len = authorizer_reply.length();
  if (policy.lossy)
    reply.flags = reply.flags | CEPH_MSG_CONNECT_LOSSY;

  set_features((uint64_t)reply.features & (uint64_t)connect.features);
  ldout(async_msgr->cct, 10) << __func__ << " accept features " << get_features() << dendl;

  session_security.reset(
      get_auth_session_handler(async_msgr->cct, connect.authorizer_protocol,
                               session_key, get_features()));

  reply_bl.append((char*)&reply, sizeof(reply));

  if (reply.authorizer_len)
    reply_bl.append(authorizer_reply.c_str(), authorizer_reply.length());

  if (reply.tag == CEPH_MSGR_TAG_SEQ) {
    uint64_t s = in_seq;
    reply_bl.append((char*)&s, sizeof(s));
  }

  lock.unlock();
  // Because "replacing" will prevent other connections preempt this addr,
  // it's safe that here we don't acquire Connection's lock
  r = async_msgr->accept_conn(this);

  inject_delay();
  
  lock.lock();
  replacing = false;
  if (r < 0) {
    ldout(async_msgr->cct, 1) << __func__ << " existing race replacing process for addr=" << peer_addr
                              << " just fail later one(this)" << dendl;
    goto fail_registered;
  }
  if (state != STATE_ACCEPTING_WAIT_CONNECT_MSG_AUTH) {
    ldout(async_msgr->cct, 1) << __func__ << " state changed while accept_conn, it must be mark_down" << dendl;
    assert(state == STATE_CLOSED || state == STATE_NONE);
    goto fail_registered;
  }

  r = try_send(reply_bl);
  if (r < 0)
    goto fail_registered;

  // notify
  dispatch_queue->queue_accept(this);
  async_msgr->ms_deliver_handle_fast_accept(this);
  once_ready = true;

  if (r == 0) {
    state = next_state;
    ldout(async_msgr->cct, 2) << __func__ << " accept write reply msg done" << dendl;
  } else {
    state = STATE_WAIT_SEND;
    state_after_send = next_state;
  }

  return 0;

 fail_registered:
  ldout(async_msgr->cct, 10) << __func__ << " accept fault after register" << dendl;
  inject_delay();

 fail:
  ldout(async_msgr->cct, 10) << __func__ << " failed to accept." << dendl;
  return -1;
}