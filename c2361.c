DoCopy(const CopyStmt *stmt, const char *queryString, uint64 *processed)
{
	CopyState	cstate;
	bool		is_from = stmt->is_from;
	bool		pipe = (stmt->filename == NULL);
	Relation	rel;
	Oid			relid;
	Node	   *query = NULL;

	/* Disallow COPY to/from file or program except to superusers. */
	if (!pipe && !superuser())
	{
		if (stmt->is_program)
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("must be superuser to COPY to or from an external program"),
					 errhint("Anyone can COPY to stdout or from stdin. "
						   "psql's \\copy command also works for anyone.")));
		else
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("must be superuser to COPY to or from a file"),
					 errhint("Anyone can COPY to stdout or from stdin. "
						   "psql's \\copy command also works for anyone.")));
	}

	if (stmt->relation)
	{
		TupleDesc	tupDesc;
		AclMode		required_access = (is_from ? ACL_INSERT : ACL_SELECT);
		RangeTblEntry *rte;
		List	   *attnums;
		ListCell   *cur;

		Assert(!stmt->query);

		/* Open and lock the relation, using the appropriate lock type. */
		rel = heap_openrv(stmt->relation,
						  (is_from ? RowExclusiveLock : AccessShareLock));

		relid = RelationGetRelid(rel);

		rte = makeNode(RangeTblEntry);
		rte->rtekind = RTE_RELATION;
		rte->relid = RelationGetRelid(rel);
		rte->relkind = rel->rd_rel->relkind;
		rte->requiredPerms = required_access;

		tupDesc = RelationGetDescr(rel);
		attnums = CopyGetAttnums(tupDesc, rel, stmt->attlist);
		foreach(cur, attnums)
		{
			int			attno = lfirst_int(cur) -
			FirstLowInvalidHeapAttributeNumber;

			if (is_from)
				rte->modifiedCols = bms_add_member(rte->modifiedCols, attno);
			else
				rte->selectedCols = bms_add_member(rte->selectedCols, attno);
		}
		ExecCheckRTPerms(list_make1(rte), true);

		/*
		 * Permission check for row security policies.
		 *
		 * check_enable_rls will ereport(ERROR) if the user has requested
		 * something invalid and will otherwise indicate if we should enable
		 * RLS (returns RLS_ENABLED) or not for this COPY statement.
		 *
		 * If the relation has a row security policy and we are to apply it
		 * then perform a "query" copy and allow the normal query processing to
		 * handle the policies.
		 *
		 * If RLS is not enabled for this, then just fall through to the
		 * normal non-filtering relation handling.
		 */
		if (check_enable_rls(rte->relid, InvalidOid) == RLS_ENABLED)
		{
			SelectStmt *select;
			ColumnRef  *cr;
			ResTarget  *target;
			RangeVar   *from;

			if (is_from)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("COPY FROM not supported with row level security."),
						 errhint("Use direct INSERT statements instead.")));

			/* Build target list */
			cr = makeNode(ColumnRef);

			if (!stmt->attlist)
				cr->fields = list_make1(makeNode(A_Star));
			else
				cr->fields = stmt->attlist;

			cr->location = 1;

			target = makeNode(ResTarget);
			target->name = NULL;
			target->indirection = NIL;
			target->val = (Node *) cr;
			target->location = 1;

			/* Build FROM clause */
			from = stmt->relation;

			/* Build query */
			select = makeNode(SelectStmt);
			select->targetList = list_make1(target);
			select->fromClause = list_make1(from);

			query = (Node*) select;

			/* Close the handle to the relation as it is no longer needed. */
			heap_close(rel, (is_from ? RowExclusiveLock : AccessShareLock));
			rel = NULL;
		}
	}
	else
	{
		Assert(stmt->query);

		query = stmt->query;
		relid = InvalidOid;
		rel = NULL;
	}

	if (is_from)
	{
		Assert(rel);

		/* check read-only transaction */
		if (XactReadOnly && !rel->rd_islocaltemp)
			PreventCommandIfReadOnly("COPY FROM");

		cstate = BeginCopyFrom(rel, stmt->filename, stmt->is_program,
							   stmt->attlist, stmt->options);
		*processed = CopyFrom(cstate);	/* copy from file to database */
		EndCopyFrom(cstate);
	}
	else
	{
		cstate = BeginCopyTo(rel, query, queryString, relid,
							 stmt->filename, stmt->is_program,
							 stmt->attlist, stmt->options);
		*processed = DoCopyTo(cstate);	/* copy from database to file */
		EndCopyTo(cstate);
	}

	/*
	 * Close the relation. If reading, we can release the AccessShareLock we
	 * got; if writing, we should hold the lock until end of transaction to
	 * ensure that updates will be committed before lock is released.
	 */
	if (rel != NULL)
		heap_close(rel, (is_from ? NoLock : AccessShareLock));

	return relid;
}