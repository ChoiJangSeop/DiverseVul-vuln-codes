ATExecAddIndex(AlteredTableInfo *tab, Relation rel,
			   IndexStmt *stmt, bool is_rebuild, LOCKMODE lockmode)
{
	bool		check_rights;
	bool		skip_build;
	bool		quiet;
	Oid			new_index;

	Assert(IsA(stmt, IndexStmt));
	Assert(!stmt->concurrent);

	/* suppress schema rights check when rebuilding existing index */
	check_rights = !is_rebuild;
	/* skip index build if phase 3 will do it or we're reusing an old one */
	skip_build = tab->rewrite || OidIsValid(stmt->oldNode);
	/* suppress notices when rebuilding existing index */
	quiet = is_rebuild;

	/* The IndexStmt has already been through transformIndexStmt */

	new_index = DefineIndex(stmt,
							InvalidOid, /* no predefined OID */
							true,		/* is_alter_table */
							check_rights,
							skip_build,
							quiet);

	/*
	 * If TryReuseIndex() stashed a relfilenode for us, we used it for the new
	 * index instead of building from scratch.	The DROP of the old edition of
	 * this index will have scheduled the storage for deletion at commit, so
	 * cancel that pending deletion.
	 */
	if (OidIsValid(stmt->oldNode))
	{
		Relation	irel = index_open(new_index, NoLock);

		RelationPreserveStorage(irel->rd_node, true);
		index_close(irel, NoLock);
	}
}