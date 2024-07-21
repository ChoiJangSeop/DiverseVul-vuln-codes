static void server_init(IRC_SERVER_REC *server)
{
	IRC_SERVER_CONNECT_REC *conn;
	char *address, *ptr, *username, *cmd;
	GTimeVal now;

	g_return_if_fail(server != NULL);

	conn = server->connrec;

	if (conn->proxy != NULL && conn->proxy_password != NULL &&
	    *conn->proxy_password != '\0') {
		cmd = g_strdup_printf("PASS %s", conn->proxy_password);
		irc_send_cmd_now(server, cmd);
		g_free(cmd);
	}

	if (conn->proxy != NULL && conn->proxy_string != NULL) {
		cmd = g_strdup_printf(conn->proxy_string, conn->address, conn->port);
		irc_send_cmd_now(server, cmd);
		g_free(cmd);
	}

	irc_send_cmd_now(server, "CAP LS");

	if (conn->password != NULL && *conn->password != '\0') {
                /* send password */
		cmd = g_strdup_printf("PASS %s", conn->password);
		irc_send_cmd_now(server, cmd);
		g_free(cmd);
	}

        /* send nick */
	cmd = g_strdup_printf("NICK %s", conn->nick);
	irc_send_cmd_now(server, cmd);
	g_free(cmd);

	/* send user/realname */
	address = server->connrec->address;
        ptr = strrchr(address, ':');
	if (ptr != NULL) {
		/* IPv6 address .. doesn't work here, use the string after
		   the last : char */
		address = ptr+1;
		if (*address == '\0')
			address = "x";
	}

	username = g_strdup(conn->username);
	ptr = strchr(username, ' ');
	if (ptr != NULL) *ptr = '\0';

	cmd = g_strdup_printf("USER %s %s %s :%s", username, username, address, conn->realname);
	irc_send_cmd_now(server, cmd);
	g_free(cmd);
	g_free(username);

	if (conn->proxy != NULL && conn->proxy_string_after != NULL) {
		cmd = g_strdup_printf(conn->proxy_string_after, conn->address, conn->port);
		irc_send_cmd_now(server, cmd);
		g_free(cmd);
	}

	server->isupport = g_hash_table_new((GHashFunc) g_istr_hash,
					    (GCompareFunc) g_istr_equal);

	/* set the standards */
	g_hash_table_insert(server->isupport, g_strdup("CHANMODES"), g_strdup("beI,k,l,imnpst"));
	g_hash_table_insert(server->isupport, g_strdup("PREFIX"), g_strdup("(ohv)@%+"));

	server->cmdcount = 0;

	/* prevent the queue from sending too early, we have a max cut off of 120 secs */
	/* this will reset to 1 sec after we get the 001 event */
	g_get_current_time(&now);
	memcpy(&((IRC_SERVER_REC *)server)->wait_cmd, &now, sizeof(GTimeVal));
	((IRC_SERVER_REC *)server)->wait_cmd.tv_sec += 120;
}