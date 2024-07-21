void irc_core_init(void)
{
	CHAT_PROTOCOL_REC *rec;

	rec = g_new0(CHAT_PROTOCOL_REC, 1);
	rec->name = "IRC";
	rec->fullname = "Internet Relay Chat";
	rec->chatnet = "ircnet";

        rec->case_insensitive = TRUE;

	rec->create_chatnet = create_chatnet;
        rec->create_server_setup = create_server_setup;
        rec->create_channel_setup = create_channel_setup;
	rec->create_server_connect = create_server_connect;
	rec->destroy_server_connect = destroy_server_connect;

	rec->server_init_connect = irc_server_init_connect;
	rec->server_connect = irc_server_connect;
	rec->channel_create =
		(CHANNEL_REC *(*) (SERVER_REC *, const char *,
				   const char *, int))
                irc_channel_create;
	rec->query_create =
		(QUERY_REC *(*) (const char *, const char *, int))
                irc_query_create;

	chat_protocol_register(rec);
        g_free(rec);

        irc_session_init();
	irc_chatnets_init();
	irc_servers_init();
	irc_channels_init();
	irc_queries_init();

	ctcp_init();
	irc_commands_init();
	irc_irc_init();
	lag_init();
	netsplit_init();
	irc_expandos_init();
	cap_init();

	settings_check();
	module_register("core", "irc");
}