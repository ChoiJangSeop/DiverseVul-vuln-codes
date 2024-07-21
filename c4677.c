static void event_nick_invalid(IRC_SERVER_REC *server, const char *data)
{
	if (!server->connected)
		server_disconnect((SERVER_REC *) server);
}