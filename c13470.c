DefineSequence(ParseState *pstate, CreateSeqStmt *seq)
{
	FormData_pg_sequence seqform;
	FormData_pg_sequence_data seqdataform;
	bool		need_seq_rewrite;
	List	   *owned_by;
	CreateStmt *stmt = makeNode(CreateStmt);
	Oid			seqoid;
	ObjectAddress address;
	Relation	rel;
	HeapTuple	tuple;
	TupleDesc	tupDesc;
	Datum		value[SEQ_COL_LASTCOL];
	bool		null[SEQ_COL_LASTCOL];
	Datum		pgs_values[Natts_pg_sequence];
	bool		pgs_nulls[Natts_pg_sequence];
	int			i;

	/* Unlogged sequences are not implemented -- not clear if useful. */
	if (seq->sequence->relpersistence == RELPERSISTENCE_UNLOGGED)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("unlogged sequences are not supported")));

	/*
	 * If if_not_exists was given and a relation with the same name already
	 * exists, bail out. (Note: we needn't check this when not if_not_exists,
	 * because DefineRelation will complain anyway.)
	 */
	if (seq->if_not_exists)
	{
		RangeVarGetAndCheckCreationNamespace(seq->sequence, NoLock, &seqoid);
		if (OidIsValid(seqoid))
		{
			ereport(NOTICE,
					(errcode(ERRCODE_DUPLICATE_TABLE),
					 errmsg("relation \"%s\" already exists, skipping",
							seq->sequence->relname)));
			return InvalidObjectAddress;
		}
	}

	/* Check and set all option values */
	init_params(pstate, seq->options, seq->for_identity, true,
				&seqform, &seqdataform,
				&need_seq_rewrite, &owned_by);

	/*
	 * Create relation (and fill value[] and null[] for the tuple)
	 */
	stmt->tableElts = NIL;
	for (i = SEQ_COL_FIRSTCOL; i <= SEQ_COL_LASTCOL; i++)
	{
		ColumnDef  *coldef = makeNode(ColumnDef);

		coldef->inhcount = 0;
		coldef->is_local = true;
		coldef->is_not_null = true;
		coldef->is_from_type = false;
		coldef->storage = 0;
		coldef->raw_default = NULL;
		coldef->cooked_default = NULL;
		coldef->collClause = NULL;
		coldef->collOid = InvalidOid;
		coldef->constraints = NIL;
		coldef->location = -1;

		null[i - 1] = false;

		switch (i)
		{
			case SEQ_COL_LASTVAL:
				coldef->typeName = makeTypeNameFromOid(INT8OID, -1);
				coldef->colname = "last_value";
				value[i - 1] = Int64GetDatumFast(seqdataform.last_value);
				break;
			case SEQ_COL_LOG:
				coldef->typeName = makeTypeNameFromOid(INT8OID, -1);
				coldef->colname = "log_cnt";
				value[i - 1] = Int64GetDatum((int64) 0);
				break;
			case SEQ_COL_CALLED:
				coldef->typeName = makeTypeNameFromOid(BOOLOID, -1);
				coldef->colname = "is_called";
				value[i - 1] = BoolGetDatum(false);
				break;
		}
		stmt->tableElts = lappend(stmt->tableElts, coldef);
	}

	stmt->relation = seq->sequence;
	stmt->inhRelations = NIL;
	stmt->constraints = NIL;
	stmt->options = NIL;
	stmt->oncommit = ONCOMMIT_NOOP;
	stmt->tablespacename = NULL;
	stmt->if_not_exists = seq->if_not_exists;

	address = DefineRelation(stmt, RELKIND_SEQUENCE, seq->ownerId, NULL, NULL);
	seqoid = address.objectId;
	Assert(seqoid != InvalidOid);

	rel = table_open(seqoid, AccessExclusiveLock);
	tupDesc = RelationGetDescr(rel);

	/* now initialize the sequence's data */
	tuple = heap_form_tuple(tupDesc, value, null);
	fill_seq_with_data(rel, tuple);

	/* process OWNED BY if given */
	if (owned_by)
		process_owned_by(rel, owned_by, seq->for_identity);

	table_close(rel, NoLock);

	/* fill in pg_sequence */
	rel = table_open(SequenceRelationId, RowExclusiveLock);
	tupDesc = RelationGetDescr(rel);

	memset(pgs_nulls, 0, sizeof(pgs_nulls));

	pgs_values[Anum_pg_sequence_seqrelid - 1] = ObjectIdGetDatum(seqoid);
	pgs_values[Anum_pg_sequence_seqtypid - 1] = ObjectIdGetDatum(seqform.seqtypid);
	pgs_values[Anum_pg_sequence_seqstart - 1] = Int64GetDatumFast(seqform.seqstart);
	pgs_values[Anum_pg_sequence_seqincrement - 1] = Int64GetDatumFast(seqform.seqincrement);
	pgs_values[Anum_pg_sequence_seqmax - 1] = Int64GetDatumFast(seqform.seqmax);
	pgs_values[Anum_pg_sequence_seqmin - 1] = Int64GetDatumFast(seqform.seqmin);
	pgs_values[Anum_pg_sequence_seqcache - 1] = Int64GetDatumFast(seqform.seqcache);
	pgs_values[Anum_pg_sequence_seqcycle - 1] = BoolGetDatum(seqform.seqcycle);

	tuple = heap_form_tuple(tupDesc, pgs_values, pgs_nulls);
	CatalogTupleInsert(rel, tuple);

	heap_freetuple(tuple);
	table_close(rel, RowExclusiveLock);

	return address;
}