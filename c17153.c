cmd_starttls_next(struct smtp_server_cmd_ctx *cmd, void *context ATTR_UNUSED)
{
	struct smtp_server_connection *conn = cmd->conn;
	struct smtp_server_command *command = cmd->cmd;
	const struct smtp_server_callbacks *callbacks = conn->callbacks;
	int ret;

	smtp_server_connection_set_state(conn, SMTP_SERVER_STATE_STARTTLS,
					 NULL);

	smtp_server_command_ref(command);
	if (callbacks != NULL && callbacks->conn_cmd_starttls != NULL)
		ret = callbacks->conn_cmd_starttls(conn->context, cmd);
	else
		ret = 1;

	smtp_server_command_add_hook(command, SMTP_SERVER_COMMAND_HOOK_DESTROY,
				     cmd_starttls_destroy, NULL);

	if (ret <= 0) {
		i_assert(ret == 0 || smtp_server_command_is_replied(command));
		/* command is waiting for external event or it failed */
		smtp_server_command_unref(&command);
		return;
	}
	if (!smtp_server_command_is_replied(command)) {
		smtp_server_reply(cmd,
			220, "2.0.0", "Begin TLS negotiation now.");
	}
	smtp_server_command_unref(&command);
}