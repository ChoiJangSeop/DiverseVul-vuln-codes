static void cmd_oper(const char *data, IRC_SERVER_REC *server)
{
	char *nick, *password, *format;
	void *free_arg;

	g_return_if_fail(data != NULL);
	if (!IS_IRC_SERVER(server) || !server->connected)
		cmd_return_error(CMDERR_NOT_CONNECTED);

	if (!cmd_get_params(data, &free_arg, 2, &nick, &password))
		return;
	if (*password == '\0') {
		/* password not given, ask it.
		   irc/core handles the /OPER when password is given */
		OPER_PASS_REC *rec;

		rec = g_new(OPER_PASS_REC, 1);
		rec->server = server;
		rec->nick = g_strdup(*nick != '\0' ? nick : server->nick);

		format = format_get_text(MODULE_NAME, NULL, server, NULL,
					 IRCTXT_ASK_OPER_PASS);

		keyboard_entry_redirect((SIGNAL_FUNC) cmd_oper_got_pass,
					format,
					ENTRY_REDIRECT_FLAG_HIDDEN, rec);
                g_free(format);

		signal_stop();
	}

	cmd_params_free(free_arg);
}