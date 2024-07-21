GET_DCC_REC *dcc_get_create(IRC_SERVER_REC *server, CHAT_DCC_REC *chat,
				   const char *nick, const char *arg)
{
	GET_DCC_REC *dcc;

	dcc = g_new0(GET_DCC_REC, 1);
	dcc->orig_type = module_get_uniq_id_str("DCC", "SEND");
	dcc->type = module_get_uniq_id_str("DCC", "GET");
	dcc->fhandle = -1;

	dcc_init_rec(DCC(dcc), server, chat, nick, arg);
        return dcc;
}