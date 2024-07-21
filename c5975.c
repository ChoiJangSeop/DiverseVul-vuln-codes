static void sig_server_setup_fill_chatnet(IRC_SERVER_CONNECT_REC *conn,
					  IRC_CHATNET_REC *ircnet)
{
	if (!IS_IRC_SERVER_CONNECT(conn))
		return;
	g_return_if_fail(IS_IRCNET(ircnet));

	if (ircnet->nick != NULL) g_free_and_null(conn->alternate_nick);
	if (ircnet->usermode != NULL) {
		g_free_and_null(conn->usermode);
		conn->usermode = g_strdup(ircnet->usermode);
	}

	if (ircnet->max_kicks > 0) conn->max_kicks = ircnet->max_kicks;
	if (ircnet->max_msgs > 0) conn->max_msgs = ircnet->max_msgs;
	if (ircnet->max_modes > 0) conn->max_modes = ircnet->max_modes;
	if (ircnet->max_whois > 0) conn->max_whois = ircnet->max_whois;

	if (ircnet->max_cmds_at_once > 0)
		conn->max_cmds_at_once = ircnet->max_cmds_at_once;
	if (ircnet->cmd_queue_speed > 0)
		conn->cmd_queue_speed = ircnet->cmd_queue_speed;
	if (ircnet->max_query_chans > 0)
		conn->max_query_chans = ircnet->max_query_chans;
}