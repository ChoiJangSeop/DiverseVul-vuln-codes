static SEND_DCC_REC *dcc_send_create(IRC_SERVER_REC *server,
				     CHAT_DCC_REC *chat,
				     const char *nick, const char *arg)
{
	SEND_DCC_REC *dcc;

	dcc = g_new0(SEND_DCC_REC, 1);
	dcc->orig_type = module_get_uniq_id_str("DCC", "GET");
	dcc->type = module_get_uniq_id_str("DCC", "SEND");
	dcc->fhandle = -1;
	dcc->queue = -1;

	dcc_init_rec(DCC(dcc), server, chat, nick, arg);
        return dcc;
}