ri_ReportViolation(const RI_ConstraintInfo *riinfo,
				   Relation pk_rel, Relation fk_rel,
				   HeapTuple violator, TupleDesc tupdesc,
				   int queryno, bool spi_err)
{
	StringInfoData key_names;
	StringInfoData key_values;
	bool		onfk;
	const int16 *attnums;
	int			idx;

	if (spi_err)
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("referential integrity query on \"%s\" from constraint \"%s\" on \"%s\" gave unexpected result",
						RelationGetRelationName(pk_rel),
						NameStr(riinfo->conname),
						RelationGetRelationName(fk_rel)),
				 errhint("This is most likely due to a rule having rewritten the query.")));

	/*
	 * Determine which relation to complain about.  If tupdesc wasn't passed
	 * by caller, assume the violator tuple came from there.
	 */
	onfk = (queryno == RI_PLAN_CHECK_LOOKUPPK);
	if (onfk)
	{
		attnums = riinfo->fk_attnums;
		if (tupdesc == NULL)
			tupdesc = fk_rel->rd_att;
	}
	else
	{
		attnums = riinfo->pk_attnums;
		if (tupdesc == NULL)
			tupdesc = pk_rel->rd_att;
	}

	/* Get printable versions of the keys involved */
	initStringInfo(&key_names);
	initStringInfo(&key_values);
	for (idx = 0; idx < riinfo->nkeys; idx++)
	{
		int			fnum = attnums[idx];
		char	   *name,
				   *val;

		name = SPI_fname(tupdesc, fnum);
		val = SPI_getvalue(violator, tupdesc, fnum);
		if (!val)
			val = "null";

		if (idx > 0)
		{
			appendStringInfoString(&key_names, ", ");
			appendStringInfoString(&key_values, ", ");
		}
		appendStringInfoString(&key_names, name);
		appendStringInfoString(&key_values, val);
	}

	if (onfk)
		ereport(ERROR,
				(errcode(ERRCODE_FOREIGN_KEY_VIOLATION),
				 errmsg("insert or update on table \"%s\" violates foreign key constraint \"%s\"",
						RelationGetRelationName(fk_rel),
						NameStr(riinfo->conname)),
				 errdetail("Key (%s)=(%s) is not present in table \"%s\".",
						   key_names.data, key_values.data,
						   RelationGetRelationName(pk_rel)),
				 errtableconstraint(fk_rel, NameStr(riinfo->conname))));
	else
		ereport(ERROR,
				(errcode(ERRCODE_FOREIGN_KEY_VIOLATION),
				 errmsg("update or delete on table \"%s\" violates foreign key constraint \"%s\" on table \"%s\"",
						RelationGetRelationName(pk_rel),
						NameStr(riinfo->conname),
						RelationGetRelationName(fk_rel)),
			errdetail("Key (%s)=(%s) is still referenced from table \"%s\".",
					  key_names.data, key_values.data,
					  RelationGetRelationName(fk_rel)),
				 errtableconstraint(fk_rel, NameStr(riinfo->conname))));
}