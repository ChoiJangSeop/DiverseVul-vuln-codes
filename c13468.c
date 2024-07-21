transformCreateStmt(CreateStmt *stmt, const char *queryString)
{
	ParseState *pstate;
	CreateStmtContext cxt;
	List	   *result;
	List	   *save_alist;
	ListCell   *elements;
	Oid			namespaceid;
	Oid			existing_relid;
	ParseCallbackState pcbstate;
	bool		is_foreign_table = IsA(stmt, CreateForeignTableStmt);

	/*
	 * We must not scribble on the passed-in CreateStmt, so copy it.  (This is
	 * overkill, but easy.)
	 */
	stmt = copyObject(stmt);

	/* Set up pstate */
	pstate = make_parsestate(NULL);
	pstate->p_sourcetext = queryString;

	/*
	 * Look up the creation namespace.  This also checks permissions on the
	 * target namespace, locks it against concurrent drops, checks for a
	 * preexisting relation in that namespace with the same name, and updates
	 * stmt->relation->relpersistence if the selected namespace is temporary.
	 */
	setup_parser_errposition_callback(&pcbstate, pstate,
									  stmt->relation->location);
	namespaceid =
		RangeVarGetAndCheckCreationNamespace(stmt->relation, NoLock,
											 &existing_relid);
	cancel_parser_errposition_callback(&pcbstate);

	/*
	 * If the relation already exists and the user specified "IF NOT EXISTS",
	 * bail out with a NOTICE.
	 */
	if (stmt->if_not_exists && OidIsValid(existing_relid))
	{
		ereport(NOTICE,
				(errcode(ERRCODE_DUPLICATE_TABLE),
				 errmsg("relation \"%s\" already exists, skipping",
						stmt->relation->relname)));
		return NIL;
	}

	/*
	 * If the target relation name isn't schema-qualified, make it so.  This
	 * prevents some corner cases in which added-on rewritten commands might
	 * think they should apply to other relations that have the same name and
	 * are earlier in the search path.  But a local temp table is effectively
	 * specified to be in pg_temp, so no need for anything extra in that case.
	 */
	if (stmt->relation->schemaname == NULL
		&& stmt->relation->relpersistence != RELPERSISTENCE_TEMP)
		stmt->relation->schemaname = get_namespace_name(namespaceid);

	/* Set up CreateStmtContext */
	cxt.pstate = pstate;
	if (IsA(stmt, CreateForeignTableStmt))
	{
		cxt.stmtType = "CREATE FOREIGN TABLE";
		cxt.isforeign = true;
	}
	else
	{
		cxt.stmtType = "CREATE TABLE";
		cxt.isforeign = false;
	}
	cxt.relation = stmt->relation;
	cxt.rel = NULL;
	cxt.inhRelations = stmt->inhRelations;
	cxt.isalter = false;
	cxt.columns = NIL;
	cxt.ckconstraints = NIL;
	cxt.fkconstraints = NIL;
	cxt.ixconstraints = NIL;
	cxt.likeclauses = NIL;
	cxt.extstats = NIL;
	cxt.blist = NIL;
	cxt.alist = NIL;
	cxt.pkey = NULL;
	cxt.ispartitioned = stmt->partspec != NULL;
	cxt.partbound = stmt->partbound;
	cxt.ofType = (stmt->ofTypename != NULL);

	Assert(!stmt->ofTypename || !stmt->inhRelations);	/* grammar enforces */

	if (stmt->ofTypename)
		transformOfType(&cxt, stmt->ofTypename);

	if (stmt->partspec)
	{
		if (stmt->inhRelations && !stmt->partbound)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("cannot create partitioned table as inheritance child")));
	}

	/*
	 * Run through each primary element in the table creation clause. Separate
	 * column defs from constraints, and do preliminary analysis.
	 */
	foreach(elements, stmt->tableElts)
	{
		Node	   *element = lfirst(elements);

		switch (nodeTag(element))
		{
			case T_ColumnDef:
				transformColumnDefinition(&cxt, (ColumnDef *) element);
				break;

			case T_Constraint:
				transformTableConstraint(&cxt, (Constraint *) element);
				break;

			case T_TableLikeClause:
				transformTableLikeClause(&cxt, (TableLikeClause *) element);
				break;

			default:
				elog(ERROR, "unrecognized node type: %d",
					 (int) nodeTag(element));
				break;
		}
	}

	/*
	 * Transfer anything we already have in cxt.alist into save_alist, to keep
	 * it separate from the output of transformIndexConstraints.  (This may
	 * not be necessary anymore, but we'll keep doing it to preserve the
	 * historical order of execution of the alist commands.)
	 */
	save_alist = cxt.alist;
	cxt.alist = NIL;

	Assert(stmt->constraints == NIL);

	/*
	 * Postprocess constraints that give rise to index definitions.
	 */
	transformIndexConstraints(&cxt);

	/*
	 * Re-consideration of LIKE clauses should happen after creation of
	 * indexes, but before creation of foreign keys.  This order is critical
	 * because a LIKE clause may attempt to create a primary key.  If there's
	 * also a pkey in the main CREATE TABLE list, creation of that will not
	 * check for a duplicate at runtime (since index_check_primary_key()
	 * expects that we rejected dups here).  Creation of the LIKE-generated
	 * pkey behaves like ALTER TABLE ADD, so it will check, but obviously that
	 * only works if it happens second.  On the other hand, we want to make
	 * pkeys before foreign key constraints, in case the user tries to make a
	 * self-referential FK.
	 */
	cxt.alist = list_concat(cxt.alist, cxt.likeclauses);

	/*
	 * Postprocess foreign-key constraints.
	 */
	transformFKConstraints(&cxt, true, false);

	/*
	 * Postprocess check constraints.
	 */
	transformCheckConstraints(&cxt, !is_foreign_table ? true : false);

	/*
	 * Postprocess extended statistics.
	 */
	transformExtendedStatistics(&cxt);

	/*
	 * Output results.
	 */
	stmt->tableElts = cxt.columns;
	stmt->constraints = cxt.ckconstraints;

	result = lappend(cxt.blist, stmt);
	result = list_concat(result, cxt.alist);
	result = list_concat(result, save_alist);

	return result;
}