pqParseInput3(PGconn *conn)
{
	char		id;
	int			msgLength;
	int			avail;

	/*
	 * Loop to parse successive complete messages available in the buffer.
	 */
	for (;;)
	{
		/*
		 * Try to read a message.  First get the type code and length. Return
		 * if not enough data.
		 */
		conn->inCursor = conn->inStart;
		if (pqGetc(&id, conn))
			return;
		if (pqGetInt(&msgLength, 4, conn))
			return;

		/*
		 * Try to validate message type/length here.  A length less than 4 is
		 * definitely broken.  Large lengths should only be believed for a few
		 * message types.
		 */
		if (msgLength < 4)
		{
			handleSyncLoss(conn, id, msgLength);
			return;
		}
		if (msgLength > 30000 && !VALID_LONG_MESSAGE_TYPE(id))
		{
			handleSyncLoss(conn, id, msgLength);
			return;
		}

		/*
		 * Can't process if message body isn't all here yet.
		 */
		msgLength -= 4;
		avail = conn->inEnd - conn->inCursor;
		if (avail < msgLength)
		{
			/*
			 * Before returning, enlarge the input buffer if needed to hold
			 * the whole message.  This is better than leaving it to
			 * pqReadData because we can avoid multiple cycles of realloc()
			 * when the message is large; also, we can implement a reasonable
			 * recovery strategy if we are unable to make the buffer big
			 * enough.
			 */
			if (pqCheckInBufferSpace(conn->inCursor + (size_t) msgLength,
									 conn))
			{
				/*
				 * XXX add some better recovery code... plan is to skip over
				 * the message using its length, then report an error. For the
				 * moment, just treat this like loss of sync (which indeed it
				 * might be!)
				 */
				handleSyncLoss(conn, id, msgLength);
			}
			return;
		}

		/*
		 * NOTIFY and NOTICE messages can happen in any state; always process
		 * them right away.
		 *
		 * Most other messages should only be processed while in BUSY state.
		 * (In particular, in READY state we hold off further parsing until
		 * the application collects the current PGresult.)
		 *
		 * However, if the state is IDLE then we got trouble; we need to deal
		 * with the unexpected message somehow.
		 *
		 * ParameterStatus ('S') messages are a special case: in IDLE state we
		 * must process 'em (this case could happen if a new value was adopted
		 * from config file due to SIGHUP), but otherwise we hold off until
		 * BUSY state.
		 */
		if (id == 'A')
		{
			if (getNotify(conn))
				return;
		}
		else if (id == 'N')
		{
			if (pqGetErrorNotice3(conn, false))
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
			 * ParameterStatus is handled normally; anything else is just
			 * dropped on the floor after displaying a suitable warning
			 * notice.	(An ERROR is very possibly the backend telling us why
			 * it is about to close the connection, so we don't want to just
			 * discard it...)
			 */
			if (id == 'E')
			{
				if (pqGetErrorNotice3(conn, false /* treat as notice */ ))
					return;
			}
			else if (id == 'S')
			{
				if (getParameterStatus(conn))
					return;
			}
			else
			{
				pqInternalNotice(&conn->noticeHooks,
						"message type 0x%02x arrived from server while idle",
								 id);
				/* Discard the unexpected message */
				conn->inCursor += msgLength;
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
					conn->asyncStatus = PGASYNC_READY;
					break;
				case 'E':		/* error return */
					if (pqGetErrorNotice3(conn, true))
						return;
					conn->asyncStatus = PGASYNC_READY;
					break;
				case 'Z':		/* backend is ready for new query */
					if (getReadyForQuery(conn))
						return;
					conn->asyncStatus = PGASYNC_IDLE;
					break;
				case 'I':		/* empty query */
					if (conn->result == NULL)
					{
						conn->result = PQmakeEmptyPGresult(conn,
														   PGRES_EMPTY_QUERY);
						if (!conn->result)
							return;
					}
					conn->asyncStatus = PGASYNC_READY;
					break;
				case '1':		/* Parse Complete */
					/* If we're doing PQprepare, we're done; else ignore */
					if (conn->queryclass == PGQUERY_PREPARE)
					{
						if (conn->result == NULL)
						{
							conn->result = PQmakeEmptyPGresult(conn,
														   PGRES_COMMAND_OK);
							if (!conn->result)
								return;
						}
						conn->asyncStatus = PGASYNC_READY;
					}
					break;
				case '2':		/* Bind Complete */
				case '3':		/* Close Complete */
					/* Nothing to do for these message types */
					break;
				case 'S':		/* parameter status */
					if (getParameterStatus(conn))
						return;
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
				case 'T':		/* Row Description */
					if (conn->result == NULL ||
						conn->queryclass == PGQUERY_DESCRIBE)
					{
						/* First 'T' in a query sequence */
						if (getRowDescriptions(conn, msgLength))
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
				case 'n':		/* No Data */

					/*
					 * NoData indicates that we will not be seeing a
					 * RowDescription message because the statement or portal
					 * inquired about doesn't return rows.
					 *
					 * If we're doing a Describe, we have to pass something
					 * back to the client, so set up a COMMAND_OK result,
					 * instead of TUPLES_OK.  Otherwise we can just ignore
					 * this message.
					 */
					if (conn->queryclass == PGQUERY_DESCRIBE)
					{
						if (conn->result == NULL)
						{
							conn->result = PQmakeEmptyPGresult(conn,
														   PGRES_COMMAND_OK);
							if (!conn->result)
								return;
						}
						conn->asyncStatus = PGASYNC_READY;
					}
					break;
				case 't':		/* Parameter Description */
					if (getParamDescriptions(conn))
						return;
					break;
				case 'D':		/* Data Row */
					if (conn->result != NULL &&
						conn->result->resultStatus == PGRES_TUPLES_OK)
					{
						/* Read another tuple of a normal query response */
						if (getAnotherTuple(conn, msgLength))
							return;
						/* getAnotherTuple() moves inStart itself */
						continue;
					}
					else if (conn->result != NULL &&
							 conn->result->resultStatus == PGRES_FATAL_ERROR)
					{
						/*
						 * We've already choked for some reason.  Just discard
						 * tuples till we get to the end of the query.
						 */
						conn->inCursor += msgLength;
					}
					else
					{
						/* Set up to report error at end of query */
						printfPQExpBuffer(&conn->errorMessage,
										  libpq_gettext("server sent data (\"D\" message) without prior row description (\"T\" message)\n"));
						pqSaveErrorResult(conn);
						/* Discard the unexpected message */
						conn->inCursor += msgLength;
					}
					break;
				case 'G':		/* Start Copy In */
					if (getCopyStart(conn, PGRES_COPY_IN))
						return;
					conn->asyncStatus = PGASYNC_COPY_IN;
					break;
				case 'H':		/* Start Copy Out */
					if (getCopyStart(conn, PGRES_COPY_OUT))
						return;
					conn->asyncStatus = PGASYNC_COPY_OUT;
					conn->copy_already_done = 0;
					break;
				case 'W':		/* Start Copy Both */
					if (getCopyStart(conn, PGRES_COPY_BOTH))
						return;
					conn->asyncStatus = PGASYNC_COPY_BOTH;
					conn->copy_already_done = 0;
					break;
				case 'd':		/* Copy Data */

					/*
					 * If we see Copy Data, just silently drop it.	This would
					 * only occur if application exits COPY OUT mode too
					 * early.
					 */
					conn->inCursor += msgLength;
					break;
				case 'c':		/* Copy Done */

					/*
					 * If we see Copy Done, just silently drop it.	This is
					 * the normal case during PQendcopy.  We will keep
					 * swallowing data, expecting to see command-complete for
					 * the COPY command.
					 */
					break;
				default:
					printfPQExpBuffer(&conn->errorMessage,
									  libpq_gettext(
													"unexpected response from server; first received character was \"%c\"\n"),
									  id);
					/* build an error result holding the error message */
					pqSaveErrorResult(conn);
					/* not sure if we will see more, so go to ready state */
					conn->asyncStatus = PGASYNC_READY;
					/* Discard the unexpected message */
					conn->inCursor += msgLength;
					break;
			}					/* switch on protocol character */
		}
		/* Successfully consumed this message */
		if (conn->inCursor == conn->inStart + 5 + msgLength)
		{
			/* Normal case: parsing agrees with specified length */
			conn->inStart = conn->inCursor;
		}
		else
		{
			/* Trouble --- report it */
			printfPQExpBuffer(&conn->errorMessage,
							  libpq_gettext("message contents do not agree with length in message type \"%c\"\n"),
							  id);
			/* build an error result holding the error message */
			pqSaveErrorResult(conn);
			conn->asyncStatus = PGASYNC_READY;
			/* trust the specified message length as what to skip */
			conn->inStart += 5 + msgLength;
		}
	}
}