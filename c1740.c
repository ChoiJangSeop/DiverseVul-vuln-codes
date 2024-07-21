ATPostAlterTypeParse(Oid oldId, char *cmd,
					 List **wqueue, LOCKMODE lockmode, bool rewrite)
{
	List	   *raw_parsetree_list;
	List	   *querytree_list;
	ListCell   *list_item;

	/*
	 * We expect that we will get only ALTER TABLE and CREATE INDEX
	 * statements. Hence, there is no need to pass them through
	 * parse_analyze() or the rewriter, but instead we need to pass them
	 * through parse_utilcmd.c to make them ready for execution.
	 */
	raw_parsetree_list = raw_parser(cmd);
	querytree_list = NIL;
	foreach(list_item, raw_parsetree_list)
	{
		Node	   *stmt = (Node *) lfirst(list_item);

		if (IsA(stmt, IndexStmt))
			querytree_list = lappend(querytree_list,
									 transformIndexStmt((IndexStmt *) stmt,
														cmd));
		else if (IsA(stmt, AlterTableStmt))
			querytree_list = list_concat(querytree_list,
							 transformAlterTableStmt((AlterTableStmt *) stmt,
													 cmd));
		else
			querytree_list = lappend(querytree_list, stmt);
	}

	/*
	 * Attach each generated command to the proper place in the work queue.
	 * Note this could result in creation of entirely new work-queue entries.
	 *
	 * Also note that we have to tweak the command subtypes, because it turns
	 * out that re-creation of indexes and constraints has to act a bit
	 * differently from initial creation.
	 */
	foreach(list_item, querytree_list)
	{
		Node	   *stm = (Node *) lfirst(list_item);
		Relation	rel;
		AlteredTableInfo *tab;

		switch (nodeTag(stm))
		{
			case T_IndexStmt:
				{
					IndexStmt  *stmt = (IndexStmt *) stm;
					AlterTableCmd *newcmd;

					if (!rewrite)
						TryReuseIndex(oldId, stmt);

					rel = relation_openrv(stmt->relation, lockmode);
					tab = ATGetQueueEntry(wqueue, rel);
					newcmd = makeNode(AlterTableCmd);
					newcmd->subtype = AT_ReAddIndex;
					newcmd->def = (Node *) stmt;
					tab->subcmds[AT_PASS_OLD_INDEX] =
						lappend(tab->subcmds[AT_PASS_OLD_INDEX], newcmd);
					relation_close(rel, NoLock);
					break;
				}
			case T_AlterTableStmt:
				{
					AlterTableStmt *stmt = (AlterTableStmt *) stm;
					ListCell   *lcmd;

					rel = relation_openrv(stmt->relation, lockmode);
					tab = ATGetQueueEntry(wqueue, rel);
					foreach(lcmd, stmt->cmds)
					{
						AlterTableCmd *cmd = (AlterTableCmd *) lfirst(lcmd);
						Constraint *con;

						switch (cmd->subtype)
						{
							case AT_AddIndex:
								Assert(IsA(cmd->def, IndexStmt));
								if (!rewrite)
									TryReuseIndex(get_constraint_index(oldId),
												  (IndexStmt *) cmd->def);
								cmd->subtype = AT_ReAddIndex;
								tab->subcmds[AT_PASS_OLD_INDEX] =
									lappend(tab->subcmds[AT_PASS_OLD_INDEX], cmd);
								break;
							case AT_AddConstraint:
								Assert(IsA(cmd->def, Constraint));
								con = (Constraint *) cmd->def;
								/* rewriting neither side of a FK */
								if (con->contype == CONSTR_FOREIGN &&
									!rewrite && !tab->rewrite)
									TryReuseForeignKey(oldId, con);
								cmd->subtype = AT_ReAddConstraint;
								tab->subcmds[AT_PASS_OLD_CONSTR] =
									lappend(tab->subcmds[AT_PASS_OLD_CONSTR], cmd);
								break;
							default:
								elog(ERROR, "unexpected statement type: %d",
									 (int) cmd->subtype);
						}
					}
					relation_close(rel, NoLock);
					break;
				}
			default:
				elog(ERROR, "unexpected statement type: %d",
					 (int) nodeTag(stm));
		}
	}
}