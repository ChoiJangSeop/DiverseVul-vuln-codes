static void l2cap_conf_rfc_get(struct l2cap_chan *chan, void *rsp, int len)
{
	int type, olen;
	unsigned long val;
	/* Use sane default values in case a misbehaving remote device
	 * did not send an RFC or extended window size option.
	 */
	u16 txwin_ext = chan->ack_win;
	struct l2cap_conf_rfc rfc = {
		.mode = chan->mode,
		.retrans_timeout = cpu_to_le16(L2CAP_DEFAULT_RETRANS_TO),
		.monitor_timeout = cpu_to_le16(L2CAP_DEFAULT_MONITOR_TO),
		.max_pdu_size = cpu_to_le16(chan->imtu),
		.txwin_size = min_t(u16, chan->ack_win, L2CAP_DEFAULT_TX_WINDOW),
	};

	BT_DBG("chan %p, rsp %p, len %d", chan, rsp, len);

	if ((chan->mode != L2CAP_MODE_ERTM) && (chan->mode != L2CAP_MODE_STREAMING))
		return;

	while (len >= L2CAP_CONF_OPT_SIZE) {
		len -= l2cap_get_conf_opt(&rsp, &type, &olen, &val);

		switch (type) {
		case L2CAP_CONF_RFC:
			if (olen != sizeof(rfc))
				break;
			memcpy(&rfc, (void *)val, olen);
			break;
		case L2CAP_CONF_EWS:
			if (olen != 2)
				break;
			txwin_ext = val;
			break;
		}
	}

	switch (rfc.mode) {
	case L2CAP_MODE_ERTM:
		chan->retrans_timeout = le16_to_cpu(rfc.retrans_timeout);
		chan->monitor_timeout = le16_to_cpu(rfc.monitor_timeout);
		chan->mps = le16_to_cpu(rfc.max_pdu_size);
		if (test_bit(FLAG_EXT_CTRL, &chan->flags))
			chan->ack_win = min_t(u16, chan->ack_win, txwin_ext);
		else
			chan->ack_win = min_t(u16, chan->ack_win,
					      rfc.txwin_size);
		break;
	case L2CAP_MODE_STREAMING:
		chan->mps    = le16_to_cpu(rfc.max_pdu_size);
	}
}