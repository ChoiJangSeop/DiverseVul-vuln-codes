TryReuseIndex(Oid oldId, IndexStmt *stmt)
{
	if (CheckIndexCompatible(oldId,
							 stmt->relation,
							 stmt->accessMethod,
							 stmt->indexParams,
							 stmt->excludeOpNames))
	{
		Relation	irel = index_open(oldId, NoLock);

		stmt->oldNode = irel->rd_node.relNode;
		index_close(irel, NoLock);
	}
}