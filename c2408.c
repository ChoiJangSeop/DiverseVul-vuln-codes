recv_password_packet(Port *port)
{
	StringInfoData buf;

	if (PG_PROTOCOL_MAJOR(port->proto) >= 3)
	{
		/* Expect 'p' message type */
		int			mtype;

		mtype = pq_getbyte();
		if (mtype != 'p')
		{
			/*
			 * If the client just disconnects without offering a password,
			 * don't make a log entry.  This is legal per protocol spec and in
			 * fact commonly done by psql, so complaining just clutters the
			 * log.
			 */
			if (mtype != EOF)
				ereport(COMMERROR,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
					errmsg("expected password response, got message type %d",
						   mtype)));
			return NULL;		/* EOF or bad message type */
		}
	}
	else
	{
		/* For pre-3.0 clients, avoid log entry if they just disconnect */
		if (pq_peekbyte() == EOF)
			return NULL;		/* EOF */
	}

	initStringInfo(&buf);
	if (pq_getmessage(&buf, 1000))		/* receive password */
	{
		/* EOF - pq_getmessage already logged a suitable message */
		pfree(buf.data);
		return NULL;
	}

	/*
	 * Apply sanity check: password packet length should agree with length of
	 * contained string.  Note it is safe to use strlen here because
	 * StringInfo is guaranteed to have an appended '\0'.
	 */
	if (strlen(buf.data) + 1 != buf.len)
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("invalid password packet size")));

	/* Do not echo password to logs, for security. */
	ereport(DEBUG5,
			(errmsg("received password packet")));

	/*
	 * Return the received string.  Note we do not attempt to do any
	 * character-set conversion on it; since we don't yet know the client's
	 * encoding, there wouldn't be much point.
	 */
	return buf.data;
}