static void sig_print_starting(TEXT_DEST_REC *dest)
{
	NETJOIN_SERVER_REC *rec;

	if (printing_joins)
		return;

	if (!IS_IRC_SERVER(dest->server))
		return;

	if (!(dest->level & MSGLEVEL_PUBLIC))
		return;

	if (!server_ischannel(dest->server, dest->target))
		return;

	rec = netjoin_find_server(IRC_SERVER(dest->server));
	if (rec != NULL && rec->netjoins != NULL)
		print_netjoins(rec, dest->target);
}