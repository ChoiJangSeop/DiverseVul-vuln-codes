static void query_remove_all(IRC_CHANNEL_REC *channel)
{
	SERVER_QUERY_REC *rec;
	int n;

	rec = channel->server->chanqueries;

	/* remove channel from query lists */
	for (n = 0; n < CHANNEL_QUERIES; n++)
		rec->queries[n] = g_slist_remove(rec->queries[n], channel);
	rec->current_queries = g_slist_remove(rec->current_queries, channel);

	query_check(channel->server);
}