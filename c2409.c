HandleFunctionRequest(StringInfo msgBuf)
{
	Oid			fid;
	AclResult	aclresult;
	FunctionCallInfoData fcinfo;
	int16		rformat;
	Datum		retval;
	struct fp_info my_fp;
	struct fp_info *fip;
	bool		callit;
	bool		was_logged = false;
	char		msec_str[32];

	/*
	 * Read message contents if not already done.
	 */
	if (PG_PROTOCOL_MAJOR(FrontendProtocol) < 3)
	{
		if (GetOldFunctionMessage(msgBuf))
		{
			if (IsTransactionState())
				ereport(COMMERROR,
						(errcode(ERRCODE_CONNECTION_FAILURE),
						 errmsg("unexpected EOF on client connection with an open transaction")));
			else
			{
				/*
				 * Can't send DEBUG log messages to client at this point.
				 * Since we're disconnecting right away, we don't need to
				 * restore whereToSendOutput.
				 */
				whereToSendOutput = DestNone;
				ereport(DEBUG1,
						(errcode(ERRCODE_CONNECTION_DOES_NOT_EXIST),
						 errmsg("unexpected EOF on client connection")));
			}
			return EOF;
		}
	}

	/*
	 * Now that we've eaten the input message, check to see if we actually
	 * want to do the function call or not.  It's now safe to ereport(); we
	 * won't lose sync with the frontend.
	 */
	if (IsAbortedTransactionBlockState())
		ereport(ERROR,
				(errcode(ERRCODE_IN_FAILED_SQL_TRANSACTION),
				 errmsg("current transaction is aborted, "
						"commands ignored until end of transaction block")));

	/*
	 * Now that we know we are in a valid transaction, set snapshot in case
	 * needed by function itself or one of the datatype I/O routines.
	 */
	PushActiveSnapshot(GetTransactionSnapshot());

	/*
	 * Begin parsing the buffer contents.
	 */
	if (PG_PROTOCOL_MAJOR(FrontendProtocol) < 3)
		(void) pq_getmsgstring(msgBuf); /* dummy string */

	fid = (Oid) pq_getmsgint(msgBuf, 4);		/* function oid */

	/*
	 * There used to be a lame attempt at caching lookup info here. Now we
	 * just do the lookups on every call.
	 */
	fip = &my_fp;
	fetch_fp_info(fid, fip);

	/* Log as soon as we have the function OID and name */
	if (log_statement == LOGSTMT_ALL)
	{
		ereport(LOG,
				(errmsg("fastpath function call: \"%s\" (OID %u)",
						fip->fname, fid)));
		was_logged = true;
	}

	/*
	 * Check permission to access and call function.  Since we didn't go
	 * through a normal name lookup, we need to check schema usage too.
	 */
	aclresult = pg_namespace_aclcheck(fip->namespace, GetUserId(), ACL_USAGE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, ACL_KIND_NAMESPACE,
					   get_namespace_name(fip->namespace));
	InvokeNamespaceSearchHook(fip->namespace, true);

	aclresult = pg_proc_aclcheck(fid, GetUserId(), ACL_EXECUTE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, ACL_KIND_PROC,
					   get_func_name(fid));
	InvokeFunctionExecuteHook(fid);

	/*
	 * Prepare function call info block and insert arguments.
	 *
	 * Note: for now we pass collation = InvalidOid, so collation-sensitive
	 * functions can't be called this way.  Perhaps we should pass
	 * DEFAULT_COLLATION_OID, instead?
	 */
	InitFunctionCallInfoData(fcinfo, &fip->flinfo, 0, InvalidOid, NULL, NULL);

	if (PG_PROTOCOL_MAJOR(FrontendProtocol) >= 3)
		rformat = parse_fcall_arguments(msgBuf, fip, &fcinfo);
	else
		rformat = parse_fcall_arguments_20(msgBuf, fip, &fcinfo);

	/* Verify we reached the end of the message where expected. */
	pq_getmsgend(msgBuf);

	/*
	 * If func is strict, must not call it for null args.
	 */
	callit = true;
	if (fip->flinfo.fn_strict)
	{
		int			i;

		for (i = 0; i < fcinfo.nargs; i++)
		{
			if (fcinfo.argnull[i])
			{
				callit = false;
				break;
			}
		}
	}

	if (callit)
	{
		/* Okay, do it ... */
		retval = FunctionCallInvoke(&fcinfo);
	}
	else
	{
		fcinfo.isnull = true;
		retval = (Datum) 0;
	}

	/* ensure we do at least one CHECK_FOR_INTERRUPTS per function call */
	CHECK_FOR_INTERRUPTS();

	SendFunctionResult(retval, fcinfo.isnull, fip->rettype, rformat);

	/* We no longer need the snapshot */
	PopActiveSnapshot();

	/*
	 * Emit duration logging if appropriate.
	 */
	switch (check_log_duration(msec_str, was_logged))
	{
		case 1:
			ereport(LOG,
					(errmsg("duration: %s ms", msec_str)));
			break;
		case 2:
			ereport(LOG,
					(errmsg("duration: %s ms  fastpath function call: \"%s\" (OID %u)",
							msec_str, fip->fname, fid)));
			break;
	}

	return 0;
}