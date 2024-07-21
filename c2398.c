SocketBackend(StringInfo inBuf)
{
	int			qtype;

	/*
	 * Get message type code from the frontend.
	 */
	qtype = pq_getbyte();

	if (qtype == EOF)			/* frontend disconnected */
	{
		if (IsTransactionState())
			ereport(COMMERROR,
					(errcode(ERRCODE_CONNECTION_FAILURE),
					 errmsg("unexpected EOF on client connection with an open transaction")));
		else
		{
			/*
			 * Can't send DEBUG log messages to client at this point. Since
			 * we're disconnecting right away, we don't need to restore
			 * whereToSendOutput.
			 */
			whereToSendOutput = DestNone;
			ereport(DEBUG1,
					(errcode(ERRCODE_CONNECTION_DOES_NOT_EXIST),
					 errmsg("unexpected EOF on client connection")));
		}
		return qtype;
	}

	/*
	 * Validate message type code before trying to read body; if we have lost
	 * sync, better to say "command unknown" than to run out of memory because
	 * we used garbage as a length word.
	 *
	 * This also gives us a place to set the doing_extended_query_message flag
	 * as soon as possible.
	 */
	switch (qtype)
	{
		case 'Q':				/* simple query */
			doing_extended_query_message = false;
			if (PG_PROTOCOL_MAJOR(FrontendProtocol) < 3)
			{
				/* old style without length word; convert */
				if (pq_getstring(inBuf))
				{
					if (IsTransactionState())
						ereport(COMMERROR,
								(errcode(ERRCODE_CONNECTION_FAILURE),
								 errmsg("unexpected EOF on client connection with an open transaction")));
					else
					{
						/*
						 * Can't send DEBUG log messages to client at this
						 * point.Since we're disconnecting right away, we
						 * don't need to restore whereToSendOutput.
						 */
						whereToSendOutput = DestNone;
						ereport(DEBUG1,
								(errcode(ERRCODE_CONNECTION_DOES_NOT_EXIST),
							 errmsg("unexpected EOF on client connection")));
					}
					return EOF;
				}
			}
			break;

		case 'F':				/* fastpath function call */
			/* we let fastpath.c cope with old-style input of this */
			doing_extended_query_message = false;
			break;

		case 'X':				/* terminate */
			doing_extended_query_message = false;
			ignore_till_sync = false;
			break;

		case 'B':				/* bind */
		case 'C':				/* close */
		case 'D':				/* describe */
		case 'E':				/* execute */
		case 'H':				/* flush */
		case 'P':				/* parse */
			doing_extended_query_message = true;
			/* these are only legal in protocol 3 */
			if (PG_PROTOCOL_MAJOR(FrontendProtocol) < 3)
				ereport(FATAL,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("invalid frontend message type %d", qtype)));
			break;

		case 'S':				/* sync */
			/* stop any active skip-till-Sync */
			ignore_till_sync = false;
			/* mark not-extended, so that a new error doesn't begin skip */
			doing_extended_query_message = false;
			/* only legal in protocol 3 */
			if (PG_PROTOCOL_MAJOR(FrontendProtocol) < 3)
				ereport(FATAL,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("invalid frontend message type %d", qtype)));
			break;

		case 'd':				/* copy data */
		case 'c':				/* copy done */
		case 'f':				/* copy fail */
			doing_extended_query_message = false;
			/* these are only legal in protocol 3 */
			if (PG_PROTOCOL_MAJOR(FrontendProtocol) < 3)
				ereport(FATAL,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("invalid frontend message type %d", qtype)));
			break;

		default:

			/*
			 * Otherwise we got garbage from the frontend.  We treat this as
			 * fatal because we have probably lost message boundary sync, and
			 * there's no good way to recover.
			 */
			ereport(FATAL,
					(errcode(ERRCODE_PROTOCOL_VIOLATION),
					 errmsg("invalid frontend message type %d", qtype)));
			break;
	}

	/*
	 * In protocol version 3, all frontend messages have a length word next
	 * after the type code; we can read the message contents independently of
	 * the type.
	 */
	if (PG_PROTOCOL_MAJOR(FrontendProtocol) >= 3)
	{
		if (pq_getmessage(inBuf, 0))
			return EOF;			/* suitable message already logged */
	}

	return qtype;
}