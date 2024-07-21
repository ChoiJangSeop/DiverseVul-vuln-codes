ReceiveCopyBegin(CopyState cstate)
{
	if (PG_PROTOCOL_MAJOR(FrontendProtocol) >= 3)
	{
		/* new way */
		StringInfoData buf;
		int			natts = list_length(cstate->attnumlist);
		int16		format = (cstate->binary ? 1 : 0);
		int			i;

		pq_beginmessage(&buf, 'G');
		pq_sendbyte(&buf, format);		/* overall format */
		pq_sendint(&buf, natts, 2);
		for (i = 0; i < natts; i++)
			pq_sendint(&buf, format, 2);		/* per-column formats */
		pq_endmessage(&buf);
		cstate->copy_dest = COPY_NEW_FE;
		cstate->fe_msgbuf = makeStringInfo();
	}
	else if (PG_PROTOCOL_MAJOR(FrontendProtocol) >= 2)
	{
		/* old way */
		if (cstate->binary)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			errmsg("COPY BINARY is not supported to stdout or from stdin")));
		pq_putemptymessage('G');
		cstate->copy_dest = COPY_OLD_FE;
	}
	else
	{
		/* very old way */
		if (cstate->binary)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			errmsg("COPY BINARY is not supported to stdout or from stdin")));
		pq_putemptymessage('D');
		cstate->copy_dest = COPY_OLD_FE;
	}
	/* We *must* flush here to ensure FE knows it can send. */
	pq_flush();
}