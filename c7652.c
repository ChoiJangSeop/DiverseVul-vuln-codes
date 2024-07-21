static void l2cap_do_start(struct l2cap_chan *chan)
{
	struct l2cap_conn *conn = chan->conn;

	if (conn->hcon->type == LE_LINK) {
		l2cap_le_start(chan);
		return;
	}

	if (!(conn->info_state & L2CAP_INFO_FEAT_MASK_REQ_SENT)) {
		l2cap_request_info(conn);
		return;
	}

	if (!(conn->info_state & L2CAP_INFO_FEAT_MASK_REQ_DONE))
		return;

	if (l2cap_chan_check_security(chan, true) &&
	    __l2cap_no_conn_pending(chan))
		l2cap_start_connection(chan);
}