destroyODBCStmt(ODBCStmt *stmt)
{
	ODBCStmt **stmtp;

	assert(isValidStmt(stmt));

	/* first set this object to invalid */
	stmt->Type = 0;

	/* remove this stmt from the dbc */
	assert(stmt->Dbc);
	assert(stmt->Dbc->FirstStmt);

	/* search for stmt in linked list */
	stmtp = &stmt->Dbc->FirstStmt;

	while (*stmtp && *stmtp != stmt)
		stmtp = &(*stmtp)->next;
	/* stmtp points to location in list where stmt is found */

	assert(*stmtp == stmt);	/* we must have found it */

	/* now remove it from the linked list */
	*stmtp = stmt->next;

	/* cleanup own managed data */
	deleteODBCErrorList(&stmt->Error);

	destroyODBCDesc(stmt->ImplParamDescr);
	destroyODBCDesc(stmt->ImplRowDescr);
	destroyODBCDesc(stmt->AutoApplParamDescr);
	destroyODBCDesc(stmt->AutoApplRowDescr);

	if (stmt->hdl)
		mapi_close_handle(stmt->hdl);

	free(stmt);
}