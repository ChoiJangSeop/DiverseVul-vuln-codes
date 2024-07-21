smtp_server_connection_handle_input(struct smtp_server_connection *conn)
{
	struct smtp_server_command *pending_command;
	enum smtp_command_parse_error error_code;
	const char *cmd_name, *cmd_params, *error;
	int ret;

	/* Check whether we are continuing a command */
	pending_command = NULL;
	if (conn->command_queue_tail != NULL) {
		pending_command =
			((conn->command_queue_tail->state ==
			  SMTP_SERVER_COMMAND_STATE_SUBMITTED_REPLY) ?
			 conn->command_queue_tail : NULL);
	}

	smtp_server_connection_timeout_reset(conn);

	/* Parse commands */
	ret = 1;
	while (!conn->closing && ret != 0) {
		while ((ret = smtp_command_parse_next(
			conn->smtp_parser, &cmd_name, &cmd_params,
			&error_code, &error)) > 0) {

			if (pending_command != NULL) {
				/* Previous command is now fully read and ready
				   to reply */
				smtp_server_command_ready_to_reply(pending_command);
				pending_command = NULL;
			}

			e_debug(conn->event, "Received new command: %s %s",
				cmd_name, cmd_params);

			conn->stats.command_count++;

			/* Handle command (cmd may be destroyed after this) */
			if (!smtp_server_connection_handle_command(conn,
				cmd_name, cmd_params))
				return;

			if (conn->disconnected)
				return;
			/* Client indicated it will close after this command;
			   stop trying to read more. */
			if (conn->closing)
				break;

			if (!smtp_server_connection_check_pipeline(conn)) {
				smtp_server_connection_input_halt(conn);
				return;
			}

			if (conn->command_queue_tail != NULL) {
				pending_command =
					((conn->command_queue_tail->state ==
					  SMTP_SERVER_COMMAND_STATE_SUBMITTED_REPLY) ?
					 conn->command_queue_tail : NULL);
			}
		}

		if (ret < 0 && conn->conn.input->eof) {
			const char *error =
				i_stream_get_disconnect_reason(conn->conn.input);
			e_debug(conn->event, "Remote closed connection: %s",
				error);

			if (conn->command_queue_head == NULL ||
			    conn->command_queue_head->state <
			    SMTP_SERVER_COMMAND_STATE_SUBMITTED_REPLY) {
				/* No pending commands or unfinished
				   command; close */
				smtp_server_connection_close(&conn, error);
			} else {
				/* A command is still processing;
				   only drop input io for now */
				conn->input_broken = TRUE;
				smtp_server_connection_input_halt(conn);
			}
			return;
		}

		if (ret < 0) {
			struct smtp_server_command *cmd;

			e_debug(conn->event,
				"Client sent invalid command: %s", error);

			switch (error_code) {
			case SMTP_COMMAND_PARSE_ERROR_BROKEN_COMMAND:
				conn->input_broken = TRUE;
				/* fall through */
			case SMTP_COMMAND_PARSE_ERROR_BAD_COMMAND:
				cmd = smtp_server_command_new_invalid(conn);
				smtp_server_command_fail(
					cmd, 500, "5.5.2",
					"Invalid command syntax");
				break;
			case SMTP_COMMAND_PARSE_ERROR_LINE_TOO_LONG:
				cmd = smtp_server_command_new_invalid(conn);
				smtp_server_command_fail(
					cmd, 500, "5.5.2", "Line too long");
				break;
			case SMTP_COMMAND_PARSE_ERROR_DATA_TOO_LARGE:
				/* Command data size exceeds the absolute limit;
				   i.e. beyond which we don't even want to skip
				   data anymore. The command error is usually
				   already submitted by the application and sent
				   to the client. */
				smtp_server_connection_close(&conn,
					"Command data size exceeds absolute limit");
				return;
			case SMTP_COMMAND_PARSE_ERROR_BROKEN_STREAM:
				smtp_server_connection_close(&conn, error);
				return;
			default:
				i_unreached();
			}
		}

		if (conn->disconnected)
			return;
		if (conn->input_broken || conn->closing) {
			smtp_server_connection_input_halt(conn);
			return;
		}

		if (ret == 0 && pending_command != NULL &&
		    !smtp_command_parser_pending_data(conn->smtp_parser)) {
			/* Previous command is now fully read and ready to
			   reply */
			smtp_server_command_ready_to_reply(pending_command);
		}
	}
}