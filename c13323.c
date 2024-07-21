CreateTableAsRelExists(CreateTableAsStmt *ctas)
{
	Oid			nspid;
	IntoClause *into = ctas->into;

	nspid = RangeVarGetCreationNamespace(into->rel);

	if (get_relname_relid(into->rel->relname, nspid))
	{
		if (!ctas->if_not_exists)
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_TABLE),
					 errmsg("relation \"%s\" already exists",
							into->rel->relname)));

		/* The relation exists and IF NOT EXISTS has been specified */
		ereport(NOTICE,
				(errcode(ERRCODE_DUPLICATE_TABLE),
				 errmsg("relation \"%s\" already exists, skipping",
						into->rel->relname)));
		return true;
	}

	/* Relation does not exist, it can be created */
	return false;
}