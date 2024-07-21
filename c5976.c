static void sig_chatnet_destroyed(IRC_CHATNET_REC *rec)
{
	if (IS_IRC_CHATNET(rec))
                g_free(rec->usermode);
}