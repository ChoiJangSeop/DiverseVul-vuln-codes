static void sasl_fail(IRC_SERVER_REC *server, const char *data, const char *from)
{
	char *params, *error;

	/* Stop any pending timeout, if any */
	if (server->sasl_timeout != 0) {
		g_source_remove(server->sasl_timeout);
		server->sasl_timeout = 0;
	}

	params = event_get_params(data, 2, NULL, &error);

	server->sasl_success = FALSE;

	signal_emit("server sasl failure", 2, server, error);

	/* Terminate the negotiation */
	cap_finish_negotiation(server);

	g_free(params);
}