void ProtocolV2::write_event() {
  ldout(cct, 10) << __func__ << dendl;
  ssize_t r = 0;

  connection->write_lock.lock();
  if (can_write) {
    if (keepalive) {
      append_keepalive();
      keepalive = false;
    }

    auto start = ceph::mono_clock::now();
    bool more;
    do {
      const auto out_entry = _get_next_outgoing();
      if (!out_entry.m) {
        break;
      }

      if (!connection->policy.lossy) {
        // put on sent list
        sent.push_back(out_entry.m);
        out_entry.m->get();
      }
      more = !out_queue.empty();
      connection->write_lock.unlock();

      // send_message or requeue messages may not encode message
      if (!out_entry.is_prepared) {
        prepare_send_message(connection->get_features(), out_entry.m);
      }

      r = write_message(out_entry.m, more);

      connection->write_lock.lock();
      if (r == 0) {
        ;
      } else if (r < 0) {
        ldout(cct, 1) << __func__ << " send msg failed" << dendl;
        break;
      } else if (r > 0)
        break;
    } while (can_write);
    write_in_progress = false;

    // if r > 0 mean data still lefted, so no need _try_send.
    if (r == 0) {
      uint64_t left = ack_left;
      if (left) {
        ceph_le64 s;
        s = in_seq;
        auto ack = AckFrame::Encode(in_seq);
        connection->outgoing_bl.append(ack.get_buffer(session_stream_handlers));
        ldout(cct, 10) << __func__ << " try send msg ack, acked " << left
                       << " messages" << dendl;
        ack_left -= left;
        left = ack_left;
        r = connection->_try_send(left);
      } else if (is_queued()) {
        r = connection->_try_send();
      }
    }
    connection->write_lock.unlock();

    connection->logger->tinc(l_msgr_running_send_time,
                             ceph::mono_clock::now() - start);
    if (r < 0) {
      ldout(cct, 1) << __func__ << " send msg failed" << dendl;
      connection->lock.lock();
      fault();
      connection->lock.unlock();
      return;
    }
  } else {
    write_in_progress = false;
    connection->write_lock.unlock();
    connection->lock.lock();
    connection->write_lock.lock();
    if (state == STANDBY && !connection->policy.server && is_queued()) {
      ldout(cct, 10) << __func__ << " policy.server is false" << dendl;
      if (server_cookie) {  // only increment connect_seq if there is a session
        connect_seq++;
      }
      connection->_connect();
    } else if (connection->cs && state != NONE && state != CLOSED &&
               state != START_CONNECT) {
      r = connection->_try_send();
      if (r < 0) {
        ldout(cct, 1) << __func__ << " send outcoming bl failed" << dendl;
        connection->write_lock.unlock();
        fault();
        connection->lock.unlock();
        return;
      }
    }
    connection->write_lock.unlock();
    connection->lock.unlock();
  }
}