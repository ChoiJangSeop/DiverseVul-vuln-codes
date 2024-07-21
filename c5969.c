static void sig_print_starting(TEXT_DEST_REC *dest)
{
	IRC_SERVER_REC *rec;

	if (printing_splits)
		return;

	if (!IS_IRC_SERVER(dest->server))
		return;

	if (!(dest->level & MSGLEVEL_PUBLIC))
		return;

	if (!server_ischannel(dest->server, dest->target))
		return;

	rec = IRC_SERVER(dest->server);
	if (rec->split_servers != NULL)
		print_splits(rec, dest->target);
}