static void sasl_step_fail(IRC_SERVER_REC *server)
{
	irc_send_cmd_now(server, "AUTHENTICATE *");
	cap_finish_negotiation(server);

	server->sasl_timeout = 0;

	signal_emit("server sasl failure", 2, server, "The server sent an invalid payload");
}