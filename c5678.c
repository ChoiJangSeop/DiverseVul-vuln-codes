static void ctcp_msg_dcc_chat(IRC_SERVER_REC *server, const char *data,
			      const char *nick, const char *addr,
			      const char *target, CHAT_DCC_REC *chat)
{
        CHAT_DCC_REC *dcc;
	char **params;
	int paramcount;
        int passive, autoallow = FALSE;

        /* CHAT <unused> <address> <port> */
	/* CHAT <unused> <address> 0 <id> (DCC CHAT passive protocol) */
	params = g_strsplit(data, " ", -1);
	paramcount = g_strv_length(params);

	if (paramcount < 3) {
		g_strfreev(params);
		return;
	}
	passive = paramcount == 4 && g_strcmp0(params[2], "0") == 0;

	dcc = DCC_CHAT(dcc_find_request(DCC_CHAT_TYPE, nick, NULL));
	if (dcc != NULL) {
		if (dcc_is_listening(dcc)) {
			/* we requested dcc chat, they requested
			   dcc chat from us .. allow it. */
			dcc_destroy(DCC(dcc));
			autoallow = TRUE;
		} else if (!dcc_is_passive(dcc)) {
			/* we already have one dcc chat request
			   from this nick, remove it. */
			dcc_destroy(DCC(dcc));
		} else if (passive) {
			if (dcc->pasv_id != atoi(params[3])) {
				/* IDs don't match! */
				dcc_destroy(DCC(dcc));
			} else {
				/* IDs are ok! Update address and port and
				   connect! */
				dcc->target = g_strdup(target);
				dcc->port = atoi(params[2]);
				dcc_str2ip(params[1], &dcc->addr);
				net_ip2host(&dcc->addr, dcc->addrstr);

				dcc_chat_connect(dcc);
				g_strfreev(params);
				return;
			}
		}
	}

	dcc = dcc_chat_create(server, chat, nick, params[0]);
	dcc->target = g_strdup(target);
	dcc->port = atoi(params[2]);

	if (passive)
		dcc->pasv_id = atoi(params[3]);

	dcc_str2ip(params[1], &dcc->addr);
	net_ip2host(&dcc->addr, dcc->addrstr);

	signal_emit("dcc request", 2, dcc, addr);

	if (autoallow || DCC_CHAT_AUTOACCEPT(dcc, server, nick, addr)) {
		if (passive) {
			/* Passive DCC... let's set up a listening socket
			   and send reply back */
			dcc_chat_passive(dcc);
		} else {
			dcc_chat_connect(dcc);
		}
	}
	g_strfreev(params);
}