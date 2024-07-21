refresh_by_match_merge(Oid matviewOid, Oid tempOid, Oid relowner,
					   int save_sec_context)
{
	StringInfoData querybuf;
	Relation	matviewRel;
	Relation	tempRel;
	char	   *matviewname;
	char	   *tempname;
	char	   *diffname;
	TupleDesc	tupdesc;
	bool		foundUniqueIndex;
	List	   *indexoidlist;
	ListCell   *indexoidscan;
	int16		relnatts;
	bool	   *usedForQual;

	initStringInfo(&querybuf);
	matviewRel = heap_open(matviewOid, NoLock);
	matviewname = quote_qualified_identifier(get_namespace_name(RelationGetNamespace(matviewRel)),
										RelationGetRelationName(matviewRel));
	tempRel = heap_open(tempOid, NoLock);
	tempname = quote_qualified_identifier(get_namespace_name(RelationGetNamespace(tempRel)),
										  RelationGetRelationName(tempRel));
	diffname = make_temptable_name_n(tempname, 2);

	relnatts = matviewRel->rd_rel->relnatts;
	usedForQual = (bool *) palloc0(sizeof(bool) * relnatts);

	/* Open SPI context. */
	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	/* Analyze the temp table with the new contents. */
	appendStringInfo(&querybuf, "ANALYZE %s", tempname);
	if (SPI_exec(querybuf.data, 0) != SPI_OK_UTILITY)
		elog(ERROR, "SPI_exec failed: %s", querybuf.data);

	/*
	 * We need to ensure that there are not duplicate rows without NULLs in
	 * the new data set before we can count on the "diff" results.  Check for
	 * that in a way that allows showing the first duplicated row found.  Even
	 * after we pass this test, a unique index on the materialized view may
	 * find a duplicate key problem.
	 */
	resetStringInfo(&querybuf);
	appendStringInfo(&querybuf,
					 "SELECT newdata FROM %s newdata "
					 "WHERE newdata IS NOT NULL AND EXISTS "
					 "(SELECT * FROM %s newdata2 WHERE newdata2 IS NOT NULL "
					 "AND newdata2 OPERATOR(pg_catalog.*=) newdata "
					 "AND newdata2.ctid OPERATOR(pg_catalog.<>) "
					 "newdata.ctid) LIMIT 1",
					 tempname, tempname);
	if (SPI_execute(querybuf.data, false, 1) != SPI_OK_SELECT)
		elog(ERROR, "SPI_exec failed: %s", querybuf.data);
	if (SPI_processed > 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_CARDINALITY_VIOLATION),
				 errmsg("new data for \"%s\" contains duplicate rows without any null columns",
						RelationGetRelationName(matviewRel)),
				 errdetail("Row: %s",
			SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1))));
	}

	SetUserIdAndSecContext(relowner,
						   save_sec_context | SECURITY_LOCAL_USERID_CHANGE);

	/* Start building the query for creating the diff table. */
	resetStringInfo(&querybuf);
	appendStringInfo(&querybuf,
					 "CREATE TEMP TABLE %s AS "
					 "SELECT mv.ctid AS tid, newdata "
					 "FROM %s mv FULL JOIN %s newdata ON (",
					 diffname, matviewname, tempname);

	/*
	 * Get the list of index OIDs for the table from the relcache, and look up
	 * each one in the pg_index syscache.  We will test for equality on all
	 * columns present in all unique indexes which only reference columns and
	 * include all rows.
	 */
	tupdesc = matviewRel->rd_att;
	foundUniqueIndex = false;
	indexoidlist = RelationGetIndexList(matviewRel);

	foreach(indexoidscan, indexoidlist)
	{
		Oid			indexoid = lfirst_oid(indexoidscan);
		Relation	indexRel;
		Form_pg_index indexStruct;

		indexRel = index_open(indexoid, RowExclusiveLock);
		indexStruct = indexRel->rd_index;

		/*
		 * We're only interested if it is unique, valid, contains no
		 * expressions, and is not partial.
		 */
		if (indexStruct->indisunique &&
			IndexIsValid(indexStruct) &&
			RelationGetIndexExpressions(indexRel) == NIL &&
			RelationGetIndexPredicate(indexRel) == NIL)
		{
			int			numatts = indexStruct->indnatts;
			int			i;

			/* Add quals for all columns from this index. */
			for (i = 0; i < numatts; i++)
			{
				int			attnum = indexStruct->indkey.values[i];
				Oid			type;
				Oid			op;
				const char *colname;

				/*
				 * Only include the column once regardless of how many times
				 * it shows up in how many indexes.
				 */
				if (usedForQual[attnum - 1])
					continue;
				usedForQual[attnum - 1] = true;

				/*
				 * Actually add the qual, ANDed with any others.
				 */
				if (foundUniqueIndex)
					appendStringInfoString(&querybuf, " AND ");

				colname = quote_identifier(NameStr((tupdesc->attrs[attnum - 1])->attname));
				appendStringInfo(&querybuf, "newdata.%s ", colname);
				type = attnumTypeId(matviewRel, attnum);
				op = lookup_type_cache(type, TYPECACHE_EQ_OPR)->eq_opr;
				mv_GenerateOper(&querybuf, op);
				appendStringInfo(&querybuf, " mv.%s", colname);

				foundUniqueIndex = true;
			}
		}

		/* Keep the locks, since we're about to run DML which needs them. */
		index_close(indexRel, NoLock);
	}

	list_free(indexoidlist);

	if (!foundUniqueIndex)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
			   errmsg("cannot refresh materialized view \"%s\" concurrently",
					  matviewname),
				 errhint("Create a unique index with no WHERE clause on one or more columns of the materialized view.")));

	appendStringInfoString(&querybuf,
						   " AND newdata OPERATOR(pg_catalog.*=) mv) "
						   "WHERE newdata IS NULL OR mv IS NULL "
						   "ORDER BY tid");

	/* Create the temporary "diff" table. */
	if (SPI_exec(querybuf.data, 0) != SPI_OK_UTILITY)
		elog(ERROR, "SPI_exec failed: %s", querybuf.data);

	SetUserIdAndSecContext(relowner,
						   save_sec_context | SECURITY_RESTRICTED_OPERATION);

	/*
	 * We have no further use for data from the "full-data" temp table, but we
	 * must keep it around because its type is referenced from the diff table.
	 */

	/* Analyze the diff table. */
	resetStringInfo(&querybuf);
	appendStringInfo(&querybuf, "ANALYZE %s", diffname);
	if (SPI_exec(querybuf.data, 0) != SPI_OK_UTILITY)
		elog(ERROR, "SPI_exec failed: %s", querybuf.data);

	OpenMatViewIncrementalMaintenance();

	/* Deletes must come before inserts; do them first. */
	resetStringInfo(&querybuf);
	appendStringInfo(&querybuf,
				   "DELETE FROM %s mv WHERE ctid OPERATOR(pg_catalog.=) ANY "
					 "(SELECT diff.tid FROM %s diff "
					 "WHERE diff.tid IS NOT NULL "
					 "AND diff.newdata IS NULL)",
					 matviewname, diffname);
	if (SPI_exec(querybuf.data, 0) != SPI_OK_DELETE)
		elog(ERROR, "SPI_exec failed: %s", querybuf.data);

	/* Inserts go last. */
	resetStringInfo(&querybuf);
	appendStringInfo(&querybuf,
					 "INSERT INTO %s SELECT (diff.newdata).* "
					 "FROM %s diff WHERE tid IS NULL",
					 matviewname, diffname);
	if (SPI_exec(querybuf.data, 0) != SPI_OK_INSERT)
		elog(ERROR, "SPI_exec failed: %s", querybuf.data);

	/* We're done maintaining the materialized view. */
	CloseMatViewIncrementalMaintenance();
	heap_close(tempRel, NoLock);
	heap_close(matviewRel, NoLock);

	/* Clean up temp tables. */
	resetStringInfo(&querybuf);
	appendStringInfo(&querybuf, "DROP TABLE %s, %s", diffname, tempname);
	if (SPI_exec(querybuf.data, 0) != SPI_OK_UTILITY)
		elog(ERROR, "SPI_exec failed: %s", querybuf.data);

	/* Close SPI context. */
	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed");
}