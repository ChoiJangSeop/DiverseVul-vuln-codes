void irc_core_deinit(void)
{
	signal_emit("chat protocol deinit", 1, chat_protocol_find("IRC"));

	cap_deinit();
	irc_expandos_deinit();
	netsplit_deinit();
	lag_deinit();
	irc_commands_deinit();
	ctcp_deinit();

	irc_queries_deinit();
	irc_channels_deinit();
	irc_irc_deinit();
	irc_servers_deinit();
	irc_chatnets_deinit();
        irc_session_deinit();

	chat_protocol_unregister("IRC");
}