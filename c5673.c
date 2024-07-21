static void sig_channel_destroyed(IRC_CHANNEL_REC *channel)
{
	g_return_if_fail(channel != NULL);

	if (IS_IRC_CHANNEL(channel) && !channel->server->disconnected &&
	    !channel->synced)
		query_remove_all(channel);
}