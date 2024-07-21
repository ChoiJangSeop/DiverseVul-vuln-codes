CHAT_DCC_REC *dcc_chat_create(IRC_SERVER_REC *server,
				     CHAT_DCC_REC *chat,
				     const char *nick, const char *arg)
{
	CHAT_DCC_REC *dcc;

	dcc = g_new0(CHAT_DCC_REC, 1);
	dcc->orig_type = dcc->type = DCC_CHAT_TYPE;
	dcc->mirc_ctcp = settings_get_bool("dcc_mirc_ctcp");
        dcc->id = dcc_chat_get_new_id(nick);

	dcc_init_rec(DCC(dcc), server, chat, nick, arg);
        return dcc;
}