static void sasl_start(IRC_SERVER_REC *server, const char *data, const char *from)
{
	IRC_SERVER_CONNECT_REC *conn;

	conn = server->connrec;

	switch (conn->sasl_mechanism) {
		case SASL_MECHANISM_PLAIN:
			irc_send_cmd_now(server, "AUTHENTICATE PLAIN");
			break;

		case SASL_MECHANISM_EXTERNAL:
			irc_send_cmd_now(server, "AUTHENTICATE EXTERNAL");
			break;
	}
	server->sasl_timeout = g_timeout_add(SASL_TIMEOUT, (GSourceFunc) sasl_timeout, server);
}