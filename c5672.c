static void check_join_failure(IRC_SERVER_REC *server, const char *channel)
{
	CHANNEL_REC *chanrec;
	char *chan2;

	if (channel[0] == '!' && channel[1] == '!')
		channel++; /* server didn't understand !channels */

	chanrec = channel_find(SERVER(server), channel);
	if (chanrec == NULL && channel[0] == '!') {
		/* it probably replied with the full !channel name,
		   find the channel with the short name.. */
		chan2 = g_strdup_printf("!%s", channel+6);
		chanrec = channel_find(SERVER(server), chan2);
		g_free(chan2);
	}

	if (chanrec != NULL && !chanrec->joined) {
		chanrec->left = TRUE;
		channel_destroy(chanrec);
	}
}