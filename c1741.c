CreateTrigger(CreateTrigStmt *stmt, const char *queryString,
			  Oid constraintOid, Oid indexOid,
			  bool isInternal)
{
	int16		tgtype;
	int			ncolumns;
	int16	   *columns;
	int2vector *tgattr;
	Node	   *whenClause;
	List	   *whenRtable;
	char	   *qual;
	Datum		values[Natts_pg_trigger];
	bool		nulls[Natts_pg_trigger];
	Relation	rel;
	AclResult	aclresult;
	Relation	tgrel;
	SysScanDesc tgscan;
	ScanKeyData key;
	Relation	pgrel;
	HeapTuple	tuple;
	Oid			fargtypes[1];	/* dummy */
	Oid			funcoid;
	Oid			funcrettype;
	Oid			trigoid;
	char		internaltrigname[NAMEDATALEN];
	char	   *trigname;
	Oid			constrrelid = InvalidOid;
	ObjectAddress myself,
				referenced;

	rel = heap_openrv(stmt->relation, AccessExclusiveLock);

	/*
	 * Triggers must be on tables or views, and there are additional
	 * relation-type-specific restrictions.
	 */
	if (rel->rd_rel->relkind == RELKIND_RELATION)
	{
		/* Tables can't have INSTEAD OF triggers */
		if (stmt->timing != TRIGGER_TYPE_BEFORE &&
			stmt->timing != TRIGGER_TYPE_AFTER)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("\"%s\" is a table",
							RelationGetRelationName(rel)),
					 errdetail("Tables cannot have INSTEAD OF triggers.")));
	}
	else if (rel->rd_rel->relkind == RELKIND_VIEW)
	{
		/*
		 * Views can have INSTEAD OF triggers (which we check below are
		 * row-level), or statement-level BEFORE/AFTER triggers.
		 */
		if (stmt->timing != TRIGGER_TYPE_INSTEAD && stmt->row)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("\"%s\" is a view",
							RelationGetRelationName(rel)),
					 errdetail("Views cannot have row-level BEFORE or AFTER triggers.")));
		/* Disallow TRUNCATE triggers on VIEWs */
		if (TRIGGER_FOR_TRUNCATE(stmt->events))
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("\"%s\" is a view",
							RelationGetRelationName(rel)),
					 errdetail("Views cannot have TRUNCATE triggers.")));
	}
	else
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("\"%s\" is not a table or view",
						RelationGetRelationName(rel))));

	if (!allowSystemTableMods && IsSystemRelation(rel))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied: \"%s\" is a system catalog",
						RelationGetRelationName(rel))));

	if (stmt->isconstraint && stmt->constrrel != NULL)
	{
		/*
		 * We must take a lock on the target relation to protect against
		 * concurrent drop.  It's not clear that AccessShareLock is strong
		 * enough, but we certainly need at least that much... otherwise, we
		 * might end up creating a pg_constraint entry referencing a
		 * nonexistent table.
		 */
		constrrelid = RangeVarGetRelid(stmt->constrrel, AccessShareLock, false);
	}

	/* permission checks */
	if (!isInternal)
	{
		aclresult = pg_class_aclcheck(RelationGetRelid(rel), GetUserId(),
									  ACL_TRIGGER);
		if (aclresult != ACLCHECK_OK)
			aclcheck_error(aclresult, ACL_KIND_CLASS,
						   RelationGetRelationName(rel));

		if (OidIsValid(constrrelid))
		{
			aclresult = pg_class_aclcheck(constrrelid, GetUserId(),
										  ACL_TRIGGER);
			if (aclresult != ACLCHECK_OK)
				aclcheck_error(aclresult, ACL_KIND_CLASS,
							   get_rel_name(constrrelid));
		}
	}

	/* Compute tgtype */
	TRIGGER_CLEAR_TYPE(tgtype);
	if (stmt->row)
		TRIGGER_SETT_ROW(tgtype);
	tgtype |= stmt->timing;
	tgtype |= stmt->events;

	/* Disallow ROW-level TRUNCATE triggers */
	if (TRIGGER_FOR_ROW(tgtype) && TRIGGER_FOR_TRUNCATE(tgtype))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("TRUNCATE FOR EACH ROW triggers are not supported")));

	/* INSTEAD triggers must be row-level, and can't have WHEN or columns */
	if (TRIGGER_FOR_INSTEAD(tgtype))
	{
		if (!TRIGGER_FOR_ROW(tgtype))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("INSTEAD OF triggers must be FOR EACH ROW")));
		if (stmt->whenClause)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("INSTEAD OF triggers cannot have WHEN conditions")));
		if (stmt->columns != NIL)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("INSTEAD OF triggers cannot have column lists")));
	}

	/*
	 * Parse the WHEN clause, if any
	 */
	if (stmt->whenClause)
	{
		ParseState *pstate;
		RangeTblEntry *rte;
		List	   *varList;
		ListCell   *lc;

		/* Set up a pstate to parse with */
		pstate = make_parsestate(NULL);
		pstate->p_sourcetext = queryString;

		/*
		 * Set up RTEs for OLD and NEW references.
		 *
		 * 'OLD' must always have varno equal to 1 and 'NEW' equal to 2.
		 */
		rte = addRangeTableEntryForRelation(pstate, rel,
											makeAlias("old", NIL),
											false, false);
		addRTEtoQuery(pstate, rte, false, true, true);
		rte = addRangeTableEntryForRelation(pstate, rel,
											makeAlias("new", NIL),
											false, false);
		addRTEtoQuery(pstate, rte, false, true, true);

		/* Transform expression.  Copy to be sure we don't modify original */
		whenClause = transformWhereClause(pstate,
										  copyObject(stmt->whenClause),
										  EXPR_KIND_TRIGGER_WHEN,
										  "WHEN");
		/* we have to fix its collations too */
		assign_expr_collations(pstate, whenClause);

		/*
		 * Check for disallowed references to OLD/NEW.
		 *
		 * NB: pull_var_clause is okay here only because we don't allow
		 * subselects in WHEN clauses; it would fail to examine the contents
		 * of subselects.
		 */
		varList = pull_var_clause(whenClause,
								  PVC_REJECT_AGGREGATES,
								  PVC_REJECT_PLACEHOLDERS);
		foreach(lc, varList)
		{
			Var		   *var = (Var *) lfirst(lc);

			switch (var->varno)
			{
				case PRS2_OLD_VARNO:
					if (!TRIGGER_FOR_ROW(tgtype))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
								 errmsg("statement trigger's WHEN condition cannot reference column values"),
								 parser_errposition(pstate, var->location)));
					if (TRIGGER_FOR_INSERT(tgtype))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
								 errmsg("INSERT trigger's WHEN condition cannot reference OLD values"),
								 parser_errposition(pstate, var->location)));
					/* system columns are okay here */
					break;
				case PRS2_NEW_VARNO:
					if (!TRIGGER_FOR_ROW(tgtype))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
								 errmsg("statement trigger's WHEN condition cannot reference column values"),
								 parser_errposition(pstate, var->location)));
					if (TRIGGER_FOR_DELETE(tgtype))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
								 errmsg("DELETE trigger's WHEN condition cannot reference NEW values"),
								 parser_errposition(pstate, var->location)));
					if (var->varattno < 0 && TRIGGER_FOR_BEFORE(tgtype))
						ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
								 errmsg("BEFORE trigger's WHEN condition cannot reference NEW system columns"),
								 parser_errposition(pstate, var->location)));
					break;
				default:
					/* can't happen without add_missing_from, so just elog */
					elog(ERROR, "trigger WHEN condition cannot contain references to other relations");
					break;
			}
		}

		/* we'll need the rtable for recordDependencyOnExpr */
		whenRtable = pstate->p_rtable;

		qual = nodeToString(whenClause);

		free_parsestate(pstate);
	}
	else
	{
		whenClause = NULL;
		whenRtable = NIL;
		qual = NULL;
	}

	/*
	 * Find and validate the trigger function.
	 */
	funcoid = LookupFuncName(stmt->funcname, 0, fargtypes, false);
	if (!isInternal)
	{
		aclresult = pg_proc_aclcheck(funcoid, GetUserId(), ACL_EXECUTE);
		if (aclresult != ACLCHECK_OK)
			aclcheck_error(aclresult, ACL_KIND_PROC,
						   NameListToString(stmt->funcname));
	}
	funcrettype = get_func_rettype(funcoid);
	if (funcrettype != TRIGGEROID)
	{
		/*
		 * We allow OPAQUE just so we can load old dump files.	When we see a
		 * trigger function declared OPAQUE, change it to TRIGGER.
		 */
		if (funcrettype == OPAQUEOID)
		{
			ereport(WARNING,
					(errmsg("changing return type of function %s from \"opaque\" to \"trigger\"",
							NameListToString(stmt->funcname))));
			SetFunctionReturnType(funcoid, TRIGGEROID);
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("function %s must return type \"trigger\"",
							NameListToString(stmt->funcname))));
	}

	/*
	 * If the command is a user-entered CREATE CONSTRAINT TRIGGER command that
	 * references one of the built-in RI_FKey trigger functions, assume it is
	 * from a dump of a pre-7.3 foreign key constraint, and take steps to
	 * convert this legacy representation into a regular foreign key
	 * constraint.	Ugly, but necessary for loading old dump files.
	 */
	if (stmt->isconstraint && !isInternal &&
		list_length(stmt->args) >= 6 &&
		(list_length(stmt->args) % 2) == 0 &&
		RI_FKey_trigger_type(funcoid) != RI_TRIGGER_NONE)
	{
		/* Keep lock on target rel until end of xact */
		heap_close(rel, NoLock);

		ConvertTriggerToFK(stmt, funcoid);

		return InvalidOid;
	}

	/*
	 * If it's a user-entered CREATE CONSTRAINT TRIGGER command, make a
	 * corresponding pg_constraint entry.
	 */
	if (stmt->isconstraint && !OidIsValid(constraintOid))
	{
		/* Internal callers should have made their own constraints */
		Assert(!isInternal);
		constraintOid = CreateConstraintEntry(stmt->trigname,
											  RelationGetNamespace(rel),
											  CONSTRAINT_TRIGGER,
											  stmt->deferrable,
											  stmt->initdeferred,
											  true,
											  RelationGetRelid(rel),
											  NULL,		/* no conkey */
											  0,
											  InvalidOid,		/* no domain */
											  InvalidOid,		/* no index */
											  InvalidOid,		/* no foreign key */
											  NULL,
											  NULL,
											  NULL,
											  NULL,
											  0,
											  ' ',
											  ' ',
											  ' ',
											  NULL,		/* no exclusion */
											  NULL,		/* no check constraint */
											  NULL,
											  NULL,
											  true,		/* islocal */
											  0,		/* inhcount */
											  true,		/* isnoinherit */
											  isInternal);		/* is_internal */
	}

	/*
	 * Generate the trigger's OID now, so that we can use it in the name if
	 * needed.
	 */
	tgrel = heap_open(TriggerRelationId, RowExclusiveLock);

	trigoid = GetNewOid(tgrel);

	/*
	 * If trigger is internally generated, modify the provided trigger name to
	 * ensure uniqueness by appending the trigger OID.	(Callers will usually
	 * supply a simple constant trigger name in these cases.)
	 */
	if (isInternal)
	{
		snprintf(internaltrigname, sizeof(internaltrigname),
				 "%s_%u", stmt->trigname, trigoid);
		trigname = internaltrigname;
	}
	else
	{
		/* user-defined trigger; use the specified trigger name as-is */
		trigname = stmt->trigname;
	}

	/*
	 * Scan pg_trigger for existing triggers on relation.  We do this only to
	 * give a nice error message if there's already a trigger of the same
	 * name.  (The unique index on tgrelid/tgname would complain anyway.) We
	 * can skip this for internally generated triggers, since the name
	 * modification above should be sufficient.
	 *
	 * NOTE that this is cool only because we have AccessExclusiveLock on the
	 * relation, so the trigger set won't be changing underneath us.
	 */
	if (!isInternal)
	{
		ScanKeyInit(&key,
					Anum_pg_trigger_tgrelid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(RelationGetRelid(rel)));
		tgscan = systable_beginscan(tgrel, TriggerRelidNameIndexId, true,
									NULL, 1, &key);
		while (HeapTupleIsValid(tuple = systable_getnext(tgscan)))
		{
			Form_pg_trigger pg_trigger = (Form_pg_trigger) GETSTRUCT(tuple);

			if (namestrcmp(&(pg_trigger->tgname), trigname) == 0)
				ereport(ERROR,
						(errcode(ERRCODE_DUPLICATE_OBJECT),
				  errmsg("trigger \"%s\" for relation \"%s\" already exists",
						 trigname, stmt->relation->relname)));
		}
		systable_endscan(tgscan);
	}

	/*
	 * Build the new pg_trigger tuple.
	 */
	memset(nulls, false, sizeof(nulls));

	values[Anum_pg_trigger_tgrelid - 1] = ObjectIdGetDatum(RelationGetRelid(rel));
	values[Anum_pg_trigger_tgname - 1] = DirectFunctionCall1(namein,
												  CStringGetDatum(trigname));
	values[Anum_pg_trigger_tgfoid - 1] = ObjectIdGetDatum(funcoid);
	values[Anum_pg_trigger_tgtype - 1] = Int16GetDatum(tgtype);
	values[Anum_pg_trigger_tgenabled - 1] = CharGetDatum(TRIGGER_FIRES_ON_ORIGIN);
	values[Anum_pg_trigger_tgisinternal - 1] = BoolGetDatum(isInternal);
	values[Anum_pg_trigger_tgconstrrelid - 1] = ObjectIdGetDatum(constrrelid);
	values[Anum_pg_trigger_tgconstrindid - 1] = ObjectIdGetDatum(indexOid);
	values[Anum_pg_trigger_tgconstraint - 1] = ObjectIdGetDatum(constraintOid);
	values[Anum_pg_trigger_tgdeferrable - 1] = BoolGetDatum(stmt->deferrable);
	values[Anum_pg_trigger_tginitdeferred - 1] = BoolGetDatum(stmt->initdeferred);

	if (stmt->args)
	{
		ListCell   *le;
		char	   *args;
		int16		nargs = list_length(stmt->args);
		int			len = 0;

		foreach(le, stmt->args)
		{
			char	   *ar = strVal(lfirst(le));

			len += strlen(ar) + 4;
			for (; *ar; ar++)
			{
				if (*ar == '\\')
					len++;
			}
		}
		args = (char *) palloc(len + 1);
		args[0] = '\0';
		foreach(le, stmt->args)
		{
			char	   *s = strVal(lfirst(le));
			char	   *d = args + strlen(args);

			while (*s)
			{
				if (*s == '\\')
					*d++ = '\\';
				*d++ = *s++;
			}
			strcpy(d, "\\000");
		}
		values[Anum_pg_trigger_tgnargs - 1] = Int16GetDatum(nargs);
		values[Anum_pg_trigger_tgargs - 1] = DirectFunctionCall1(byteain,
													  CStringGetDatum(args));
	}
	else
	{
		values[Anum_pg_trigger_tgnargs - 1] = Int16GetDatum(0);
		values[Anum_pg_trigger_tgargs - 1] = DirectFunctionCall1(byteain,
														CStringGetDatum(""));
	}

	/* build column number array if it's a column-specific trigger */
	ncolumns = list_length(stmt->columns);
	if (ncolumns == 0)
		columns = NULL;
	else
	{
		ListCell   *cell;
		int			i = 0;

		columns = (int16 *) palloc(ncolumns * sizeof(int16));
		foreach(cell, stmt->columns)
		{
			char	   *name = strVal(lfirst(cell));
			int16		attnum;
			int			j;

			/* Lookup column name.	System columns are not allowed */
			attnum = attnameAttNum(rel, name, false);
			if (attnum == InvalidAttrNumber)
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_COLUMN),
					errmsg("column \"%s\" of relation \"%s\" does not exist",
						   name, RelationGetRelationName(rel))));

			/* Check for duplicates */
			for (j = i - 1; j >= 0; j--)
			{
				if (columns[j] == attnum)
					ereport(ERROR,
							(errcode(ERRCODE_DUPLICATE_COLUMN),
							 errmsg("column \"%s\" specified more than once",
									name)));
			}

			columns[i++] = attnum;
		}
	}
	tgattr = buildint2vector(columns, ncolumns);
	values[Anum_pg_trigger_tgattr - 1] = PointerGetDatum(tgattr);

	/* set tgqual if trigger has WHEN clause */
	if (qual)
		values[Anum_pg_trigger_tgqual - 1] = CStringGetTextDatum(qual);
	else
		nulls[Anum_pg_trigger_tgqual - 1] = true;

	tuple = heap_form_tuple(tgrel->rd_att, values, nulls);

	/* force tuple to have the desired OID */
	HeapTupleSetOid(tuple, trigoid);

	/*
	 * Insert tuple into pg_trigger.
	 */
	simple_heap_insert(tgrel, tuple);

	CatalogUpdateIndexes(tgrel, tuple);

	heap_freetuple(tuple);
	heap_close(tgrel, RowExclusiveLock);

	pfree(DatumGetPointer(values[Anum_pg_trigger_tgname - 1]));
	pfree(DatumGetPointer(values[Anum_pg_trigger_tgargs - 1]));
	pfree(DatumGetPointer(values[Anum_pg_trigger_tgattr - 1]));

	/*
	 * Update relation's pg_class entry.  Crucial side-effect: other backends
	 * (and this one too!) are sent SI message to make them rebuild relcache
	 * entries.
	 */
	pgrel = heap_open(RelationRelationId, RowExclusiveLock);
	tuple = SearchSysCacheCopy1(RELOID,
								ObjectIdGetDatum(RelationGetRelid(rel)));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for relation %u",
			 RelationGetRelid(rel));

	((Form_pg_class) GETSTRUCT(tuple))->relhastriggers = true;

	simple_heap_update(pgrel, &tuple->t_self, tuple);

	CatalogUpdateIndexes(pgrel, tuple);

	heap_freetuple(tuple);
	heap_close(pgrel, RowExclusiveLock);

	/*
	 * We used to try to update the rel's relcache entry here, but that's
	 * fairly pointless since it will happen as a byproduct of the upcoming
	 * CommandCounterIncrement...
	 */

	/*
	 * Record dependencies for trigger.  Always place a normal dependency on
	 * the function.
	 */
	myself.classId = TriggerRelationId;
	myself.objectId = trigoid;
	myself.objectSubId = 0;

	referenced.classId = ProcedureRelationId;
	referenced.objectId = funcoid;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	if (isInternal && OidIsValid(constraintOid))
	{
		/*
		 * Internally-generated trigger for a constraint, so make it an
		 * internal dependency of the constraint.  We can skip depending on
		 * the relation(s), as there'll be an indirect dependency via the
		 * constraint.
		 */
		referenced.classId = ConstraintRelationId;
		referenced.objectId = constraintOid;
		referenced.objectSubId = 0;
		recordDependencyOn(&myself, &referenced, DEPENDENCY_INTERNAL);
	}
	else
	{
		/*
		 * User CREATE TRIGGER, so place dependencies.	We make trigger be
		 * auto-dropped if its relation is dropped or if the FK relation is
		 * dropped.  (Auto drop is compatible with our pre-7.3 behavior.)
		 */
		referenced.classId = RelationRelationId;
		referenced.objectId = RelationGetRelid(rel);
		referenced.objectSubId = 0;
		recordDependencyOn(&myself, &referenced, DEPENDENCY_AUTO);
		if (OidIsValid(constrrelid))
		{
			referenced.classId = RelationRelationId;
			referenced.objectId = constrrelid;
			referenced.objectSubId = 0;
			recordDependencyOn(&myself, &referenced, DEPENDENCY_AUTO);
		}
		/* Not possible to have an index dependency in this case */
		Assert(!OidIsValid(indexOid));

		/*
		 * If it's a user-specified constraint trigger, make the constraint
		 * internally dependent on the trigger instead of vice versa.
		 */
		if (OidIsValid(constraintOid))
		{
			referenced.classId = ConstraintRelationId;
			referenced.objectId = constraintOid;
			referenced.objectSubId = 0;
			recordDependencyOn(&referenced, &myself, DEPENDENCY_INTERNAL);
		}
	}

	/* If column-specific trigger, add normal dependencies on columns */
	if (columns != NULL)
	{
		int			i;

		referenced.classId = RelationRelationId;
		referenced.objectId = RelationGetRelid(rel);
		for (i = 0; i < ncolumns; i++)
		{
			referenced.objectSubId = columns[i];
			recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
		}
	}

	/*
	 * If it has a WHEN clause, add dependencies on objects mentioned in the
	 * expression (eg, functions, as well as any columns used).
	 */
	if (whenClause != NULL)
		recordDependencyOnExpr(&myself, whenClause, whenRtable,
							   DEPENDENCY_NORMAL);

	/* Post creation hook for new trigger */
	InvokeObjectPostCreateHookArg(TriggerRelationId, trigoid, 0,
								  isInternal);

	/* Keep lock on target rel until end of xact */
	heap_close(rel, NoLock);

	return trigoid;
}