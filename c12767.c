newODBCStmt(ODBCDbc *dbc)
{
	ODBCStmt *stmt = (ODBCStmt *) malloc(sizeof(ODBCStmt));

	assert(dbc);
	assert(dbc->mid);

	if (stmt == NULL) {
		/* Memory allocation error */
		addDbcError(dbc, "HY001", NULL, 0);
		return NULL;
	}

	*stmt = (ODBCStmt) {
		.Dbc = dbc,
		.Error = NULL,
		.RetrievedErrors = 0,

		.State = INITED,
		.hdl = mapi_new_handle(dbc->mid),
		.currentRow = 0,
		.startRow = 0,
		.rowSetSize = 0,
		.queryid = -1,
		.nparams = 0,
		.querytype = -1,
		.rowcount = 0,

		.qtimeout = dbc->qtimeout, /* inherit query timeout */

		.cursorType = SQL_CURSOR_FORWARD_ONLY,
		.cursorScrollable = SQL_NONSCROLLABLE,
		.retrieveData = SQL_RD_ON,
		.noScan = SQL_NOSCAN_OFF,

		.ApplRowDescr = newODBCDesc(dbc),
		.ApplParamDescr = newODBCDesc(dbc),
		.ImplRowDescr = newODBCDesc(dbc),
		.ImplParamDescr = newODBCDesc(dbc),

		.Type = ODBC_STMT_MAGIC_NR,	/* set it valid */
	};

	if (stmt->hdl == NULL) {
		/* Memory allocation error */
		addDbcError(dbc, "HY001", NULL, 0);
		destroyODBCStmt(stmt);
		return NULL;
	}
	if (stmt->ApplRowDescr == NULL || stmt->ApplParamDescr == NULL ||
	    stmt->ImplRowDescr == NULL || stmt->ImplParamDescr == NULL) {
		destroyODBCStmt(stmt);
		return NULL;
	}

	stmt->ApplRowDescr->sql_desc_alloc_type = SQL_DESC_ALLOC_AUTO;
	stmt->ApplParamDescr->sql_desc_alloc_type = SQL_DESC_ALLOC_AUTO;
	stmt->ImplRowDescr->sql_desc_alloc_type = SQL_DESC_ALLOC_AUTO;
	stmt->ImplParamDescr->sql_desc_alloc_type = SQL_DESC_ALLOC_AUTO;
	stmt->ImplRowDescr->Stmt = stmt;
	stmt->ImplParamDescr->Stmt = stmt;
	stmt->AutoApplRowDescr = stmt->ApplRowDescr;
	stmt->AutoApplParamDescr = stmt->ApplParamDescr;

	/* add this stmt to the administrative linked stmt list */
	stmt->next = dbc->FirstStmt,
	dbc->FirstStmt = stmt;

	return stmt;
}