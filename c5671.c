static void ctcp_msg_dcc_send(IRC_SERVER_REC *server, const char *data,
			      const char *nick, const char *addr,
			      const char *target, CHAT_DCC_REC *chat)
{
	GET_DCC_REC *dcc;
	SEND_DCC_REC *temp_dcc;
	IPADDR ip;
	char *address, **params, *fname;
	int paramcount, fileparams;
	int port, len, quoted = FALSE;
        uoff_t size;
	int p_id = -1;
	int passive = FALSE;

	if (addr == NULL) {
		addr = "";
	}

	/* SEND <file name> <address> <port> <size> [...] */
	/* SEND <file name> <address> 0 <size> <id> (DCC SEND passive protocol) */
	params = g_strsplit(data, " ", -1);
	paramcount = g_strv_length(params);

	if (paramcount < 4) {
		signal_emit("dcc error ctcp", 5, "SEND", data,
			    nick, addr, target);
		g_strfreev(params);
                return;
	}

	fileparams = get_file_params_count(params, paramcount);

	address = g_strdup(params[fileparams]);
	dcc_str2ip(address, &ip);
	port = atoi(params[fileparams+1]);
	size = str_to_uofft(params[fileparams+2]);

	/* If this DCC uses passive protocol then store the id for later use. */
	if (paramcount == fileparams + 4) {
		p_id = atoi(params[fileparams+3]);
		passive = TRUE;
	}

	fname = get_file_name(params, fileparams);
	g_strfreev(params);

        len = strlen(fname);
	if (len > 1 && *fname == '"' && fname[len-1] == '"') {
		/* "file name" - MIRC sends filenames with spaces like this */
		fname[len-1] = '\0';
		g_memmove(fname, fname+1, len);
                quoted = TRUE;
	}

	if (passive && port != 0) {
		/* This is NOT a DCC SEND request! This is a reply to our
		   passive request. We MUST check the IDs and then connect to
		   the remote host. */

		temp_dcc = DCC_SEND(dcc_find_request(DCC_SEND_TYPE, nick, fname));
		if (temp_dcc != NULL && p_id == temp_dcc->pasv_id) {
			temp_dcc->target = g_strdup(target);
			temp_dcc->port = port;
			temp_dcc->size = size;
			temp_dcc->file_quoted = quoted;

			memcpy(&temp_dcc->addr, &ip, sizeof(IPADDR));
			if (temp_dcc->addr.family == AF_INET)
				net_ip2host(&temp_dcc->addr, temp_dcc->addrstr);
			else {
				/* with IPv6, show it to us as it was sent */
				g_strlcpy(temp_dcc->addrstr, address,
					  sizeof(temp_dcc->addrstr));
			}

			/* This new signal is added to let us invoke
			   dcc_send_connect() which is found in dcc-send.c */
			signal_emit("dcc reply send pasv", 1, temp_dcc);
			g_free(address);
			g_free(fname);
			return;
		} else if (temp_dcc != NULL && p_id != temp_dcc->pasv_id) {
			/* IDs don't match... remove the old DCC SEND and
			   return */
			dcc_destroy(DCC(temp_dcc));
			g_free(address);
			g_free(fname);
			return;
		}
	}

	dcc = DCC_GET(dcc_find_request(DCC_GET_TYPE, nick, fname));
	if (dcc != NULL)
		dcc_destroy(DCC(dcc)); /* remove the old DCC */

	dcc = dcc_get_create(server, chat, nick, fname);
	dcc->target = g_strdup(target);

	if (passive && port == 0)
		dcc->pasv_id = p_id; /* Assign the ID to the DCC */

	memcpy(&dcc->addr, &ip, sizeof(ip));
	if (dcc->addr.family == AF_INET)
		net_ip2host(&dcc->addr, dcc->addrstr);
	else {
		/* with IPv6, show it to us as it was sent */
		g_strlcpy(dcc->addrstr, address, sizeof(dcc->addrstr));
	}
	dcc->port = port;
	dcc->size = size;
	dcc->file_quoted = quoted;

	signal_emit("dcc request", 2, dcc, addr);

	g_free(address);
	g_free(fname);
}