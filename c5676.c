static void cmd_dcc_chat(const char *data, IRC_SERVER_REC *server)
{
	void *free_arg;
	CHAT_DCC_REC *dcc;
	IPADDR own_ip;
	GIOChannel *handle;
	GHashTable *optlist;
	int p_id;
	char *nick, host[MAX_IP_LEN];
	int port;

	g_return_if_fail(data != NULL);

	if (!cmd_get_params(data, &free_arg, 1 | PARAM_FLAG_OPTIONS,
			    "dcc chat", &optlist, &nick))
		return;

	if (*nick == '\0') {
		dcc = DCC_CHAT(dcc_find_request_latest(DCC_CHAT_TYPE));
		if (dcc != NULL) {
			if (!dcc_is_passive(dcc))
				dcc_chat_connect(dcc);
			else
				dcc_chat_passive(dcc);
		}
		cmd_params_free(free_arg);
		return;
	}

	dcc = dcc_chat_find_id(nick);
	if (dcc != NULL && dcc_is_waiting_user(dcc)) {
		if (!dcc_is_passive(dcc)) {
			/* found from dcc chat requests,
			   we're the connecting side */
			dcc_chat_connect(dcc);
		} else {
			/* We are accepting a passive DCC CHAT. */
			dcc_chat_passive(dcc);
		}
		return;
	}

	if (dcc != NULL && dcc_is_listening(dcc) &&
	    dcc->server == server) {
		/* sending request again even while old request is
		   still waiting, remove it. */
		dcc_destroy(DCC(dcc));
	}

	if (!IS_IRC_SERVER(server) || !server->connected)
		cmd_param_error(CMDERR_NOT_CONNECTED);

	dcc = dcc_chat_create(server, NULL, nick, "chat");

	if (g_hash_table_lookup(optlist, "passive") == NULL) {
		/* Standard DCC CHAT... let's listen for incoming connections */
		handle = dcc_listen(net_sendbuffer_handle(server->handle),
				    &own_ip, &port);
		if (handle == NULL)
			cmd_param_error(CMDERR_ERRNO);

		dcc->handle = handle;
		dcc->tagconn =
			g_input_add(dcc->handle, G_INPUT_READ,
				    (GInputFunction) dcc_chat_listen, dcc);

		/* send the chat request */
		signal_emit("dcc request send", 1, dcc);

		dcc_ip2str(&own_ip, host);
		irc_send_cmdv(server, "PRIVMSG %s :\001DCC CHAT CHAT %s %d\001",
			      nick, host, port);
	} else {
		/* Passive protocol... we want the other side to listen */
		/* send the chat request */
		dcc->port = 0;
		signal_emit("dcc request send", 1, dcc);

		/* generate a random id */
		p_id = rand() % 64;
		dcc->pasv_id = p_id;

		/* 16843009 is the long format of 1.1.1.1, we use a fake IP
		   since the other side shouldn't care of it: they will send
		   the address for us to connect to in the reply */
		irc_send_cmdv(server,
			      "PRIVMSG %s :\001DCC CHAT CHAT 16843009 0 %d\001",
			      nick, p_id);
	}
	cmd_params_free(free_arg);
}