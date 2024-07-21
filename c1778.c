pqParseInput2(PGconn *conn)
{
	char		id;

	/*
	 * Loop to parse successive complete messages available in the buffer.
	 */
	for (;;)
	{
		/*
		 * Quit if in COPY_OUT state: we expect raw data from the server until
		 * PQendcopy is called.  Don't try to parse it according to the normal
		 * protocol.  (This is bogus.  The data lines ought to be part of the
		 * protocol and have identifying leading characters.)
		 */
		if (conn->asyncStatus == PGASYNC_COPY_OUT)
			return;

		/*
		 * OK to try to read a message type code.
		 */
		conn->inCursor = conn->inStart;
		if (pqGetc(&id, conn))
			return;

		/*
		 * NOTIFY and NOTICE messages can happen in any state besides COPY
		 * OUT; always process them right away.
		 *
		 * Most other messages should only be processed while in BUSY state.
		 * (In particular, in READY state we hold off further parsing until
		 * the application collects the current PGresult.)
		 *
		 * However, if the state is IDLE then we got trouble; we need to deal
		 * with the unexpected message somehow.
		 */
		if (id == 'A')
		{
			if (getNotify(conn))
				return;
		}
		else if (id == 'N')
		{
			if (pqGetErrorNotice2(conn, false))
				return;
		}
		else if (conn->asyncStatus != PGASYNC_BUSY)
		{
			/* If not IDLE state, just wait ... */
			if (conn->asyncStatus != PGASYNC_IDLE)
				return;

			/*
			 * Unexpected message in IDLE state; need to recover somehow.
			 * ERROR messages are displayed using the notice processor;
			 * anything else is just dropped on the floor after displaying a
			 * suitable warning notice.  (An ERROR is very possibly the
			 * backend telling us why it is about to close the connection, so
			 * we don't want to just discard it...)
			 */
			if (id == 'E')
			{
				if (pqGetErrorNotice2(conn, false /* treat as notice */ ))
					return;
			}
			else
			{
				pqInternalNotice(&conn->noticeHooks,
						"message type 0x%02x arrived from server while idle",
								 id);
				/* Discard the unexpected message; good idea?? */
				conn->inStart = conn->inEnd;
				break;
			}
		}
		else
		{
			/*
			 * In BUSY state, we can process everything.
			 */
			switch (id)
			{
				case 'C':		/* command complete */
					if (pqGets(&conn->workBuffer, conn))
						return;
					if (conn->result == NULL)
					{
						conn->result = PQmakeEmptyPGresult(conn,
														   PGRES_COMMAND_OK);
						if (!conn->result)
							return;
					}
					strncpy(conn->result->cmdStatus, conn->workBuffer.data,
							CMDSTATUS_LEN);
					checkXactStatus(conn, conn->workBuffer.data);
					conn->asyncStatus = PGASYNC_READY;
					break;
				case 'E':		/* error return */
					if (pqGetErrorNotice2(conn, true))
						return;
					conn->asyncStatus = PGASYNC_READY;
					break;
				case 'Z':		/* backend is ready for new query */
					conn->asyncStatus = PGASYNC_IDLE;
					break;
				case 'I':		/* empty query */
					/* read and throw away the closing '\0' */
					if (pqGetc(&id, conn))
						return;
					if (id != '\0')
						pqInternalNotice(&conn->noticeHooks,
										 "unexpected character %c following empty query response (\"I\" message)",
										 id);
					if (conn->result == NULL)
						conn->result = PQmakeEmptyPGresult(conn,
														   PGRES_EMPTY_QUERY);
					conn->asyncStatus = PGASYNC_READY;
					break;
				case 'K':		/* secret key data from the backend */

					/*
					 * This is expected only during backend startup, but it's
					 * just as easy to handle it as part of the main loop.
					 * Save the data and continue processing.
					 */
					if (pqGetInt(&(conn->be_pid), 4, conn))
						return;
					if (pqGetInt(&(conn->be_key), 4, conn))
						return;
					break;
				case 'P':		/* synchronous (normal) portal */
					if (pqGets(&conn->workBuffer, conn))
						return;
					/* We pretty much ignore this message type... */
					break;
				case 'T':		/* row descriptions (start of query results) */
					if (conn->result == NULL)
					{
						/* First 'T' in a query sequence */
						if (getRowDescriptions(conn))
							return;
						/* getRowDescriptions() moves inStart itself */
						continue;
					}
					else
					{
						/*
						 * A new 'T' message is treated as the start of
						 * another PGresult.  (It is not clear that this is
						 * really possible with the current backend.) We stop
						 * parsing until the application accepts the current
						 * result.
						 */
						conn->asyncStatus = PGASYNC_READY;
						return;
					}
					break;
				case 'D':		/* ASCII data tuple */
					if (conn->result != NULL)
					{
						/* Read another tuple of a normal query response */
						if (getAnotherTuple(conn, FALSE))
							return;
						/* getAnotherTuple() moves inStart itself */
						continue;
					}
					else
					{
						pqInternalNotice(&conn->noticeHooks,
										 "server sent data (\"D\" message) without prior row description (\"T\" message)");
						/* Discard the unexpected message; good idea?? */
						conn->inStart = conn->inEnd;
						return;
					}
					break;
				case 'B':		/* Binary data tuple */
					if (conn->result != NULL)
					{
						/* Read another tuple of a normal query response */
						if (getAnotherTuple(conn, TRUE))
							return;
						/* getAnotherTuple() moves inStart itself */
						continue;
					}
					else
					{
						pqInternalNotice(&conn->noticeHooks,
										 "server sent binary data (\"B\" message) without prior row description (\"T\" message)");
						/* Discard the unexpected message; good idea?? */
						conn->inStart = conn->inEnd;
						return;
					}
					break;
				case 'G':		/* Start Copy In */
					conn->asyncStatus = PGASYNC_COPY_IN;
					break;
				case 'H':		/* Start Copy Out */
					conn->asyncStatus = PGASYNC_COPY_OUT;
					break;

					/*
					 * Don't need to process CopyBothResponse here because it
					 * never arrives from the server during protocol 2.0.
					 */
				default:
					printfPQExpBuffer(&conn->errorMessage,
									  libpq_gettext(
													"unexpected response from server; first received character was \"%c\"\n"),
									  id);
					/* build an error result holding the error message */
					pqSaveErrorResult(conn);
					/* Discard the unexpected message; good idea?? */
					conn->inStart = conn->inEnd;
					conn->asyncStatus = PGASYNC_READY;
					return;
			}					/* switch on protocol character */
		}
		/* Successfully consumed this message */
		conn->inStart = conn->inCursor;
	}
}