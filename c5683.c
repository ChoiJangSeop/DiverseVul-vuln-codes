static int dcc_send_one_file(int queue, const char *target, const char *fname,
			     IRC_SERVER_REC *server, CHAT_DCC_REC *chat,
			     int passive)
{
	struct stat st;
	char *str;
	char host[MAX_IP_LEN];
	int hfile, port = 0;
        SEND_DCC_REC *dcc;
	IPADDR own_ip;
	GIOChannel *handle;

	if (dcc_find_request(DCC_SEND_TYPE, target, fname)) {
		signal_emit("dcc error send exists", 2, target, fname);
		return FALSE;
	}

	str = dcc_send_get_file(fname);
	hfile = open(str, O_RDONLY);
	g_free(str);

	if (hfile == -1) {
		signal_emit("dcc error file open", 3, target, fname,
			    GINT_TO_POINTER(errno));
		return FALSE;
	}

	if (fstat(hfile, &st) < 0) {
		g_warning("fstat() failed: %s", strerror(errno));
		close(hfile);
		return FALSE;
	}

	/* start listening (only if passive == FALSE )*/

	if (passive == FALSE) {
		handle = dcc_listen(chat != NULL ? chat->handle :
				    net_sendbuffer_handle(server->handle),
				    &own_ip, &port);
		if (handle == NULL) {
			close(hfile);
			g_warning("dcc_listen() failed: %s", strerror(errno));
			return FALSE;
		}
	} else {
		handle = NULL;
	}

	str = g_path_get_basename(fname);

	/* Replace all the spaces with underscore so that lesser
	   intelligent clients can communicate.. */
	if (settings_get_bool("dcc_send_replace_space_with_underscore"))
		g_strdelimit(str, " ", '_');

	dcc = dcc_send_create(server, chat, target, str);
	g_free(str);

	dcc->handle = handle;
	dcc->port = port;
	dcc->size = st.st_size;
	dcc->fhandle = hfile;
	dcc->queue = queue;
        dcc->file_quoted = strchr(fname, ' ') != NULL;
	if (!passive) {
		dcc->tagconn = g_input_add(handle, G_INPUT_READ,
					   (GInputFunction) dcc_send_connected,
					   dcc);
	}

	/* Generate an ID for this send if using passive protocol */
	if (passive) {
		dcc->pasv_id = rand() % 64;
	}

	/* send DCC request */
	signal_emit("dcc request send", 1, dcc);


	dcc_ip2str(&own_ip, host);
	if (passive == FALSE) {
		str = g_strdup_printf(dcc->file_quoted ?
				      "DCC SEND \"%s\" %s %d %"PRIuUOFF_T :
				      "DCC SEND %s %s %d %"PRIuUOFF_T,
				      dcc->arg, host, port, dcc->size);
	} else {
		str = g_strdup_printf(dcc->file_quoted ?
				      "DCC SEND \"%s\" 16843009 0 %"PRIuUOFF_T" %d" :
				      "DCC SEND %s 16843009 0 %"PRIuUOFF_T" %d",
				      dcc->arg, dcc->size, dcc->pasv_id);
	}
	dcc_ctcp_message(server, target, chat, FALSE, str);

	g_free(str);
	return TRUE;
}