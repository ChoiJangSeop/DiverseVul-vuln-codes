CreateStatistics(CreateStatsStmt *stmt)
{
	int16		attnums[STATS_MAX_DIMENSIONS];
	int			numcols = 0;
	char	   *namestr;
	NameData	stxname;
	Oid			statoid;
	Oid			namespaceId;
	Oid			stxowner = GetUserId();
	HeapTuple	htup;
	Datum		values[Natts_pg_statistic_ext];
	bool		nulls[Natts_pg_statistic_ext];
	int2vector *stxkeys;
	Relation	statrel;
	Relation	rel = NULL;
	Oid			relid;
	ObjectAddress parentobject,
				myself;
	Datum		types[2];		/* one for each possible type of statistic */
	int			ntypes;
	ArrayType  *stxkind;
	bool		build_ndistinct;
	bool		build_dependencies;
	bool		requested_type = false;
	int			i;
	ListCell   *cell;

	Assert(IsA(stmt, CreateStatsStmt));

	/*
	 * Examine the FROM clause.  Currently, we only allow it to be a single
	 * simple table, but later we'll probably allow multiple tables and JOIN
	 * syntax.  The grammar is already prepared for that, so we have to check
	 * here that what we got is what we can support.
	 */
	if (list_length(stmt->relations) != 1)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("only a single relation is allowed in CREATE STATISTICS")));

	foreach(cell, stmt->relations)
	{
		Node	   *rln = (Node *) lfirst(cell);

		if (!IsA(rln, RangeVar))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("only a single relation is allowed in CREATE STATISTICS")));

		/*
		 * CREATE STATISTICS will influence future execution plans but does
		 * not interfere with currently executing plans.  So it should be
		 * enough to take only ShareUpdateExclusiveLock on relation,
		 * conflicting with ANALYZE and other DDL that sets statistical
		 * information, but not with normal queries.
		 */
		rel = relation_openrv((RangeVar *) rln, ShareUpdateExclusiveLock);

		/* Restrict to allowed relation types */
		if (rel->rd_rel->relkind != RELKIND_RELATION &&
			rel->rd_rel->relkind != RELKIND_MATVIEW &&
			rel->rd_rel->relkind != RELKIND_FOREIGN_TABLE &&
			rel->rd_rel->relkind != RELKIND_PARTITIONED_TABLE)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("relation \"%s\" is not a table, foreign table, or materialized view",
							RelationGetRelationName(rel))));

		/* You must own the relation to create stats on it */
		if (!pg_class_ownercheck(RelationGetRelid(rel), stxowner))
			aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_CLASS,
						   RelationGetRelationName(rel));

		/* Creating statistics on system catalogs is not allowed */
		if (!allowSystemTableMods && IsSystemRelation(rel))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied: \"%s\" is a system catalog",
							RelationGetRelationName(rel))));
	}

	Assert(rel);
	relid = RelationGetRelid(rel);

	/*
	 * If the node has a name, split it up and determine creation namespace.
	 * If not (a possibility not considered by the grammar, but one which can
	 * occur via the "CREATE TABLE ... (LIKE)" command), then we put the
	 * object in the same namespace as the relation, and cons up a name for it.
	 */
	if (stmt->defnames)
		namespaceId = QualifiedNameGetCreationNamespace(stmt->defnames,
														&namestr);
	else
	{
		namespaceId = RelationGetNamespace(rel);
		namestr = ChooseExtendedStatisticName(RelationGetRelationName(rel),
											  ChooseExtendedStatisticNameAddition(stmt->exprs),
											  "stat",
											  namespaceId);
	}
	namestrcpy(&stxname, namestr);

	/*
	 * Deal with the possibility that the statistics object already exists.
	 */
	if (SearchSysCacheExists2(STATEXTNAMENSP,
							  CStringGetDatum(namestr),
							  ObjectIdGetDatum(namespaceId)))
	{
		if (stmt->if_not_exists)
		{
			ereport(NOTICE,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("statistics object \"%s\" already exists, skipping",
							namestr)));
			relation_close(rel, NoLock);
			return InvalidObjectAddress;
		}

		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_OBJECT),
				 errmsg("statistics object \"%s\" already exists", namestr)));
	}

	/*
	 * Currently, we only allow simple column references in the expression
	 * list.  That will change someday, and again the grammar already supports
	 * it so we have to enforce restrictions here.  For now, we can convert
	 * the expression list to a simple array of attnums.  While at it, enforce
	 * some constraints.
	 */
	foreach(cell, stmt->exprs)
	{
		Node	   *expr = (Node *) lfirst(cell);
		ColumnRef  *cref;
		char	   *attname;
		HeapTuple	atttuple;
		Form_pg_attribute attForm;
		TypeCacheEntry *type;

		if (!IsA(expr, ColumnRef))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("only simple column references are allowed in CREATE STATISTICS")));
		cref = (ColumnRef *) expr;

		if (list_length(cref->fields) != 1)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("only simple column references are allowed in CREATE STATISTICS")));
		attname = strVal((Value *) linitial(cref->fields));

		atttuple = SearchSysCacheAttName(relid, attname);
		if (!HeapTupleIsValid(atttuple))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_COLUMN),
					 errmsg("column \"%s\" does not exist",
							attname)));
		attForm = (Form_pg_attribute) GETSTRUCT(atttuple);

		/* Disallow use of system attributes in extended stats */
		if (attForm->attnum <= 0)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("statistics creation on system columns is not supported")));

		/* Disallow data types without a less-than operator */
		type = lookup_type_cache(attForm->atttypid, TYPECACHE_LT_OPR);
		if (type->lt_opr == InvalidOid)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("column \"%s\" cannot be used in statistics because its type %s has no default btree operator class",
							attname, format_type_be(attForm->atttypid))));

		/* Make sure no more than STATS_MAX_DIMENSIONS columns are used */
		if (numcols >= STATS_MAX_DIMENSIONS)
			ereport(ERROR,
					(errcode(ERRCODE_TOO_MANY_COLUMNS),
					 errmsg("cannot have more than %d columns in statistics",
							STATS_MAX_DIMENSIONS)));

		attnums[numcols] = attForm->attnum;
		numcols++;
		ReleaseSysCache(atttuple);
	}

	/*
	 * Check that at least two columns were specified in the statement. The
	 * upper bound was already checked in the loop above.
	 */
	if (numcols < 2)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
				 errmsg("extended statistics require at least 2 columns")));

	/*
	 * Sort the attnums, which makes detecting duplicates somewhat easier, and
	 * it does not hurt (it does not affect the efficiency, unlike for
	 * indexes, for example).
	 */
	qsort(attnums, numcols, sizeof(int16), compare_int16);

	/*
	 * Check for duplicates in the list of columns. The attnums are sorted so
	 * just check consecutive elements.
	 */
	for (i = 1; i < numcols; i++)
	{
		if (attnums[i] == attnums[i - 1])
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_COLUMN),
					 errmsg("duplicate column name in statistics definition")));
	}

	/* Form an int2vector representation of the sorted column list */
	stxkeys = buildint2vector(attnums, numcols);

	/*
	 * Parse the statistics kinds.
	 */
	build_ndistinct = false;
	build_dependencies = false;
	foreach(cell, stmt->stat_types)
	{
		char	   *type = strVal((Value *) lfirst(cell));

		if (strcmp(type, "ndistinct") == 0)
		{
			build_ndistinct = true;
			requested_type = true;
		}
		else if (strcmp(type, "dependencies") == 0)
		{
			build_dependencies = true;
			requested_type = true;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("unrecognized statistics kind \"%s\"",
							type)));
	}
	/* If no statistic type was specified, build them all. */
	if (!requested_type)
	{
		build_ndistinct = true;
		build_dependencies = true;
	}

	/* construct the char array of enabled statistic types */
	ntypes = 0;
	if (build_ndistinct)
		types[ntypes++] = CharGetDatum(STATS_EXT_NDISTINCT);
	if (build_dependencies)
		types[ntypes++] = CharGetDatum(STATS_EXT_DEPENDENCIES);
	Assert(ntypes > 0 && ntypes <= lengthof(types));
	stxkind = construct_array(types, ntypes, CHAROID, 1, true, 'c');

	/*
	 * Everything seems fine, so let's build the pg_statistic_ext tuple.
	 */
	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));
	values[Anum_pg_statistic_ext_stxrelid - 1] = ObjectIdGetDatum(relid);
	values[Anum_pg_statistic_ext_stxname - 1] = NameGetDatum(&stxname);
	values[Anum_pg_statistic_ext_stxnamespace - 1] = ObjectIdGetDatum(namespaceId);
	values[Anum_pg_statistic_ext_stxowner - 1] = ObjectIdGetDatum(stxowner);
	values[Anum_pg_statistic_ext_stxkeys - 1] = PointerGetDatum(stxkeys);
	values[Anum_pg_statistic_ext_stxkind - 1] = PointerGetDatum(stxkind);

	/* no statistics built yet */
	nulls[Anum_pg_statistic_ext_stxndistinct - 1] = true;
	nulls[Anum_pg_statistic_ext_stxdependencies - 1] = true;

	/* insert it into pg_statistic_ext */
	statrel = heap_open(StatisticExtRelationId, RowExclusiveLock);
	htup = heap_form_tuple(statrel->rd_att, values, nulls);
	statoid = CatalogTupleInsert(statrel, htup);
	heap_freetuple(htup);
	relation_close(statrel, RowExclusiveLock);

	/*
	 * Invalidate relcache so that others see the new statistics object.
	 */
	CacheInvalidateRelcache(rel);

	relation_close(rel, NoLock);

	/*
	 * Add an AUTO dependency on each column used in the stats, so that the
	 * stats object goes away if any or all of them get dropped.
	 */
	ObjectAddressSet(myself, StatisticExtRelationId, statoid);

	for (i = 0; i < numcols; i++)
	{
		ObjectAddressSubSet(parentobject, RelationRelationId, relid, attnums[i]);
		recordDependencyOn(&myself, &parentobject, DEPENDENCY_AUTO);
	}

	/*
	 * Also add dependencies on namespace and owner.  These are required
	 * because the stats object might have a different namespace and/or owner
	 * than the underlying table(s).
	 */
	ObjectAddressSet(parentobject, NamespaceRelationId, namespaceId);
	recordDependencyOn(&myself, &parentobject, DEPENDENCY_NORMAL);

	recordDependencyOnOwner(StatisticExtRelationId, statoid, stxowner);

	/*
	 * XXX probably there should be a recordDependencyOnCurrentExtension call
	 * here too, but we'd have to add support for ALTER EXTENSION ADD/DROP
	 * STATISTICS, which is more work than it seems worth.
	 */

	/* Return stats object's address */
	return myself;
}