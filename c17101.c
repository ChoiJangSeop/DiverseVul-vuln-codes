static bool imap_client_input_next_cmd(struct client *_client)
{
	struct imap_client *client = (struct imap_client *)_client;
	const struct imap_arg *args;
	bool parsed;
	int ret;

	if (strcasecmp(client->cmd_name, "AUTHENTICATE") == 0) {
		/* SASL-IR may need more space than input buffer's size,
		   so we'll handle it as a special case. */
		ret = cmd_authenticate(client, &parsed);
		if (ret == 0 && !parsed)
			return FALSE;
	} else if (strcasecmp(client->cmd_name, "ID") == 0) {
		/* ID extensions allows max. 30 parameters,
		   each max. 1024 bytes long. that brings us over the input
		   buffer's size, so handle the parameters one at a time */
		ret = cmd_id(client);
		if (ret == 0)
			return FALSE;
		if (ret < 0)
			ret = 1; /* don't send the error reply again */
	} else {
		ret = client_parse_command(client, &args);
		if (ret < 0)
			return TRUE;
		if (ret == 0)
			return FALSE;
		ret = *client->cmd_tag == '\0' ? -1 :
			client_command_execute(client, client->cmd_name, args);
	}

	client->cmd_finished = TRUE;
	if (ret == -2 && strcasecmp(client->cmd_tag, "LOGIN") == 0) {
		client_send_reply(&client->common, IMAP_CMD_REPLY_BAD,
			"First parameter in line is IMAP's command tag, "
			"not the command name. Add that before the command, "
			"like: a login user pass");
	} else if (ret < 0) {
		if (*client->cmd_tag == '\0')
			client->cmd_tag = "*";
		if (++client->common.bad_counter >= CLIENT_MAX_BAD_COMMANDS) {
			client_send_reply(&client->common, IMAP_CMD_REPLY_BYE,
				"Too many invalid IMAP commands.");
			client_destroy(&client->common,
				"Disconnected: Too many invalid commands");
			return FALSE;
		}
		client_send_reply(&client->common, IMAP_CMD_REPLY_BAD,
			"Error in IMAP command received by server.");
	}

	return ret != 0 && !client->common.destroyed;
}