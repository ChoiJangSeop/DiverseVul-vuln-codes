static void l2cap_do_create(struct l2cap_chan *chan, int result,
			    u8 local_amp_id, u8 remote_amp_id)
{
	BT_DBG("chan %p state %s %u -> %u", chan, state_to_string(chan->state),
	       local_amp_id, remote_amp_id);

	chan->fcs = L2CAP_FCS_NONE;

	/* Outgoing channel on AMP */
	if (chan->state == BT_CONNECT) {
		if (result == L2CAP_CR_SUCCESS) {
			chan->local_amp_id = local_amp_id;
			l2cap_send_create_chan_req(chan, remote_amp_id);
		} else {
			/* Revert to BR/EDR connect */
			l2cap_send_conn_req(chan);
		}

		return;
	}

	/* Incoming channel on AMP */
	if (__l2cap_no_conn_pending(chan)) {
		struct l2cap_conn_rsp rsp;
		char buf[128];
		rsp.scid = cpu_to_le16(chan->dcid);
		rsp.dcid = cpu_to_le16(chan->scid);

		if (result == L2CAP_CR_SUCCESS) {
			/* Send successful response */
			rsp.result = cpu_to_le16(L2CAP_CR_SUCCESS);
			rsp.status = cpu_to_le16(L2CAP_CS_NO_INFO);
		} else {
			/* Send negative response */
			rsp.result = cpu_to_le16(L2CAP_CR_NO_MEM);
			rsp.status = cpu_to_le16(L2CAP_CS_NO_INFO);
		}

		l2cap_send_cmd(chan->conn, chan->ident, L2CAP_CREATE_CHAN_RSP,
			       sizeof(rsp), &rsp);

		if (result == L2CAP_CR_SUCCESS) {
			l2cap_state_change(chan, BT_CONFIG);
			set_bit(CONF_REQ_SENT, &chan->conf_state);
			l2cap_send_cmd(chan->conn, l2cap_get_ident(chan->conn),
				       L2CAP_CONF_REQ,
				       l2cap_build_conf_req(chan, buf), buf);
			chan->num_conf_req++;
		}
	}
}