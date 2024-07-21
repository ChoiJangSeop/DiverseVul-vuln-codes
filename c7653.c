static void l2cap_conn_start(struct l2cap_conn *conn)
{
	struct l2cap_chan *chan, *tmp;

	BT_DBG("conn %p", conn);

	mutex_lock(&conn->chan_lock);

	list_for_each_entry_safe(chan, tmp, &conn->chan_l, list) {
		l2cap_chan_lock(chan);

		if (chan->chan_type != L2CAP_CHAN_CONN_ORIENTED) {
			l2cap_chan_ready(chan);
			l2cap_chan_unlock(chan);
			continue;
		}

		if (chan->state == BT_CONNECT) {
			if (!l2cap_chan_check_security(chan, true) ||
			    !__l2cap_no_conn_pending(chan)) {
				l2cap_chan_unlock(chan);
				continue;
			}

			if (!l2cap_mode_supported(chan->mode, conn->feat_mask)
			    && test_bit(CONF_STATE2_DEVICE,
					&chan->conf_state)) {
				l2cap_chan_close(chan, ECONNRESET);
				l2cap_chan_unlock(chan);
				continue;
			}

			l2cap_start_connection(chan);

		} else if (chan->state == BT_CONNECT2) {
			struct l2cap_conn_rsp rsp;
			char buf[128];
			rsp.scid = cpu_to_le16(chan->dcid);
			rsp.dcid = cpu_to_le16(chan->scid);

			if (l2cap_chan_check_security(chan, false)) {
				if (test_bit(FLAG_DEFER_SETUP, &chan->flags)) {
					rsp.result = cpu_to_le16(L2CAP_CR_PEND);
					rsp.status = cpu_to_le16(L2CAP_CS_AUTHOR_PEND);
					chan->ops->defer(chan);

				} else {
					l2cap_state_change(chan, BT_CONFIG);
					rsp.result = cpu_to_le16(L2CAP_CR_SUCCESS);
					rsp.status = cpu_to_le16(L2CAP_CS_NO_INFO);
				}
			} else {
				rsp.result = cpu_to_le16(L2CAP_CR_PEND);
				rsp.status = cpu_to_le16(L2CAP_CS_AUTHEN_PEND);
			}

			l2cap_send_cmd(conn, chan->ident, L2CAP_CONN_RSP,
				       sizeof(rsp), &rsp);

			if (test_bit(CONF_REQ_SENT, &chan->conf_state) ||
			    rsp.result != L2CAP_CR_SUCCESS) {
				l2cap_chan_unlock(chan);
				continue;
			}

			set_bit(CONF_REQ_SENT, &chan->conf_state);
			l2cap_send_cmd(conn, l2cap_get_ident(conn), L2CAP_CONF_REQ,
				       l2cap_build_conf_req(chan, buf, sizeof(buf)), buf);
			chan->num_conf_req++;
		}

		l2cap_chan_unlock(chan);
	}

	mutex_unlock(&conn->chan_lock);
}