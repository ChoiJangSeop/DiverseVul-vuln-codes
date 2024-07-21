static void print_splits(IRC_SERVER_REC *server, const char *channel)
{
	TEMP_SPLIT_REC temp;
	GSList *servers;

	printing_splits = TRUE;

	servers = g_slist_copy(server->split_servers);
	while (servers != NULL) {
		NETSPLIT_SERVER_REC *sserver = servers->data;

		/* get all the splitted servers that have the same
		   source server */
                temp.servers = get_source_servers(sserver->server, &servers);
                temp.server_rec = server;
		temp.channels = NULL;

		g_hash_table_foreach(server->splits,
				     (GHFunc) get_server_splits, &temp);
		print_server_splits(server, &temp, channel);

		g_slist_foreach(temp.channels,
				(GFunc) temp_split_chan_free, NULL);
		g_slist_free(temp.servers);
		g_slist_free(temp.channels);
	}

	printing_splits = FALSE;
}