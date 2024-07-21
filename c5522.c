static int l2cap_build_conf_req(struct l2cap_chan *chan, void *data)
{
	struct l2cap_conf_req *req = data;
	struct l2cap_conf_rfc rfc = { .mode = chan->mode };
	void *ptr = req->data;
	u16 size;

	BT_DBG("chan %p", chan);

	if (chan->num_conf_req || chan->num_conf_rsp)
		goto done;

	switch (chan->mode) {
	case L2CAP_MODE_STREAMING:
	case L2CAP_MODE_ERTM:
		if (test_bit(CONF_STATE2_DEVICE, &chan->conf_state))
			break;

		if (__l2cap_efs_supported(chan->conn))
			set_bit(FLAG_EFS_ENABLE, &chan->flags);

		/* fall through */
	default:
		chan->mode = l2cap_select_mode(rfc.mode, chan->conn->feat_mask);
		break;
	}

done:
	if (chan->imtu != L2CAP_DEFAULT_MTU)
		l2cap_add_conf_opt(&ptr, L2CAP_CONF_MTU, 2, chan->imtu);

	switch (chan->mode) {
	case L2CAP_MODE_BASIC:
		if (disable_ertm)
			break;

		if (!(chan->conn->feat_mask & L2CAP_FEAT_ERTM) &&
		    !(chan->conn->feat_mask & L2CAP_FEAT_STREAMING))
			break;

		rfc.mode            = L2CAP_MODE_BASIC;
		rfc.txwin_size      = 0;
		rfc.max_transmit    = 0;
		rfc.retrans_timeout = 0;
		rfc.monitor_timeout = 0;
		rfc.max_pdu_size    = 0;

		l2cap_add_conf_opt(&ptr, L2CAP_CONF_RFC, sizeof(rfc),
				   (unsigned long) &rfc);
		break;

	case L2CAP_MODE_ERTM:
		rfc.mode            = L2CAP_MODE_ERTM;
		rfc.max_transmit    = chan->max_tx;

		__l2cap_set_ertm_timeouts(chan, &rfc);

		size = min_t(u16, L2CAP_DEFAULT_MAX_PDU_SIZE, chan->conn->mtu -
			     L2CAP_EXT_HDR_SIZE - L2CAP_SDULEN_SIZE -
			     L2CAP_FCS_SIZE);
		rfc.max_pdu_size = cpu_to_le16(size);

		l2cap_txwin_setup(chan);

		rfc.txwin_size = min_t(u16, chan->tx_win,
				       L2CAP_DEFAULT_TX_WINDOW);

		l2cap_add_conf_opt(&ptr, L2CAP_CONF_RFC, sizeof(rfc),
				   (unsigned long) &rfc);

		if (test_bit(FLAG_EFS_ENABLE, &chan->flags))
			l2cap_add_opt_efs(&ptr, chan);

		if (test_bit(FLAG_EXT_CTRL, &chan->flags))
			l2cap_add_conf_opt(&ptr, L2CAP_CONF_EWS, 2,
					   chan->tx_win);

		if (chan->conn->feat_mask & L2CAP_FEAT_FCS)
			if (chan->fcs == L2CAP_FCS_NONE ||
			    test_bit(CONF_RECV_NO_FCS, &chan->conf_state)) {
				chan->fcs = L2CAP_FCS_NONE;
				l2cap_add_conf_opt(&ptr, L2CAP_CONF_FCS, 1,
						   chan->fcs);
			}
		break;

	case L2CAP_MODE_STREAMING:
		l2cap_txwin_setup(chan);
		rfc.mode            = L2CAP_MODE_STREAMING;
		rfc.txwin_size      = 0;
		rfc.max_transmit    = 0;
		rfc.retrans_timeout = 0;
		rfc.monitor_timeout = 0;

		size = min_t(u16, L2CAP_DEFAULT_MAX_PDU_SIZE, chan->conn->mtu -
			     L2CAP_EXT_HDR_SIZE - L2CAP_SDULEN_SIZE -
			     L2CAP_FCS_SIZE);
		rfc.max_pdu_size = cpu_to_le16(size);

		l2cap_add_conf_opt(&ptr, L2CAP_CONF_RFC, sizeof(rfc),
				   (unsigned long) &rfc);

		if (test_bit(FLAG_EFS_ENABLE, &chan->flags))
			l2cap_add_opt_efs(&ptr, chan);

		if (chan->conn->feat_mask & L2CAP_FEAT_FCS)
			if (chan->fcs == L2CAP_FCS_NONE ||
			    test_bit(CONF_RECV_NO_FCS, &chan->conf_state)) {
				chan->fcs = L2CAP_FCS_NONE;
				l2cap_add_conf_opt(&ptr, L2CAP_CONF_FCS, 1,
						   chan->fcs);
			}
		break;
	}

	req->dcid  = cpu_to_le16(chan->dcid);
	req->flags = cpu_to_le16(0);

	return ptr - data;
}