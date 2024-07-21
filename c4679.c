QUERY_REC *irc_query_find(IRC_SERVER_REC *server, const char *nick)
{
	GSList *tmp;

	for (tmp = server->queries; tmp != NULL; tmp = tmp->next) {
		QUERY_REC *rec = tmp->data;

		if (server->nick_comp_func(rec->name, nick) == 0)
			return rec;
	}

	return NULL;
}