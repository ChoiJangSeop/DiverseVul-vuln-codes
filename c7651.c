static void l2cap_security_cfm(struct hci_conn *hcon, u8 status, u8 encrypt)
{
	struct l2cap_conn *conn = hcon->l2cap_data;
	struct l2cap_chan *chan;

	if (!conn)
		return;

	BT_DBG("conn %p status 0x%2.2x encrypt %u", conn, status, encrypt);

	mutex_lock(&conn->chan_lock);

	list_for_each_entry(chan, &conn->chan_l, list) {
		l2cap_chan_lock(chan);

		BT_DBG("chan %p scid 0x%4.4x state %s", chan, chan->scid,
		       state_to_string(chan->state));

		if (chan->scid == L2CAP_CID_A2MP) {
			l2cap_chan_unlock(chan);
			continue;
		}

		if (!status && encrypt)
			chan->sec_level = hcon->sec_level;

		if (!__l2cap_no_conn_pending(chan)) {
			l2cap_chan_unlock(chan);
			continue;
		}

		if (!status && (chan->state == BT_CONNECTED ||
				chan->state == BT_CONFIG)) {
			chan->ops->resume(chan);
			l2cap_check_encryption(chan, encrypt);
			l2cap_chan_unlock(chan);
			continue;
		}

		if (chan->state == BT_CONNECT) {
			if (!status)
				l2cap_start_connection(chan);
			else
				__set_chan_timer(chan, L2CAP_DISC_TIMEOUT);
		} else if (chan->state == BT_CONNECT2 &&
			   chan->mode != L2CAP_MODE_LE_FLOWCTL) {
			struct l2cap_conn_rsp rsp;
			__u16 res, stat;

			if (!status) {
				if (test_bit(FLAG_DEFER_SETUP, &chan->flags)) {
					res = L2CAP_CR_PEND;
					stat = L2CAP_CS_AUTHOR_PEND;
					chan->ops->defer(chan);
				} else {
					l2cap_state_change(chan, BT_CONFIG);
					res = L2CAP_CR_SUCCESS;
					stat = L2CAP_CS_NO_INFO;
				}
			} else {
				l2cap_state_change(chan, BT_DISCONN);
				__set_chan_timer(chan, L2CAP_DISC_TIMEOUT);
				res = L2CAP_CR_SEC_BLOCK;
				stat = L2CAP_CS_NO_INFO;
			}

			rsp.scid   = cpu_to_le16(chan->dcid);
			rsp.dcid   = cpu_to_le16(chan->scid);
			rsp.result = cpu_to_le16(res);
			rsp.status = cpu_to_le16(stat);
			l2cap_send_cmd(conn, chan->ident, L2CAP_CONN_RSP,
				       sizeof(rsp), &rsp);

			if (!test_bit(CONF_REQ_SENT, &chan->conf_state) &&
			    res == L2CAP_CR_SUCCESS) {
				char buf[128];
				set_bit(CONF_REQ_SENT, &chan->conf_state);
				l2cap_send_cmd(conn, l2cap_get_ident(conn),
					       L2CAP_CONF_REQ,
					       l2cap_build_conf_req(chan, buf, sizeof(buf)),
					       buf);
				chan->num_conf_req++;
			}
		}

		l2cap_chan_unlock(chan);
	}

	mutex_unlock(&conn->chan_lock);
}