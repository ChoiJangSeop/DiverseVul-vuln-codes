transformAlterTableStmt(AlterTableStmt *stmt, const char *queryString)
{
	Relation	rel;
	ParseState *pstate;
	CreateStmtContext cxt;
	List	   *result;
	List	   *save_alist;
	ListCell   *lcmd,
			   *l;
	List	   *newcmds = NIL;
	bool		skipValidation = true;
	AlterTableCmd *newcmd;
	LOCKMODE	lockmode;

	/*
	 * We must not scribble on the passed-in AlterTableStmt, so copy it. (This
	 * is overkill, but easy.)
	 */
	stmt = (AlterTableStmt *) copyObject(stmt);

	/*
	 * Determine the appropriate lock level for this list of subcommands.
	 */
	lockmode = AlterTableGetLockLevel(stmt->cmds);

	/*
	 * Acquire appropriate lock on the target relation, which will be held
	 * until end of transaction.  This ensures any decisions we make here
	 * based on the state of the relation will still be good at execution. We
	 * must get lock now because execution will later require it; taking a
	 * lower grade lock now and trying to upgrade later risks deadlock.  Any
	 * new commands we add after this must not upgrade the lock level
	 * requested here.
	 */
	rel = relation_openrv_extended(stmt->relation, lockmode, stmt->missing_ok);
	if (rel == NULL)
	{
		/* this message is consistent with relation_openrv */
		ereport(NOTICE,
				(errmsg("relation \"%s\" does not exist, skipping",
						stmt->relation->relname)));
		return NIL;
	}

	/* Set up pstate and CreateStmtContext */
	pstate = make_parsestate(NULL);
	pstate->p_sourcetext = queryString;

	cxt.pstate = pstate;
	if (stmt->relkind == OBJECT_FOREIGN_TABLE)
	{
		cxt.stmtType = "ALTER FOREIGN TABLE";
		cxt.isforeign = true;
	}
	else
	{
		cxt.stmtType = "ALTER TABLE";
		cxt.isforeign = false;
	}
	cxt.relation = stmt->relation;
	cxt.rel = rel;
	cxt.inhRelations = NIL;
	cxt.isalter = true;
	cxt.hasoids = false;		/* need not be right */
	cxt.columns = NIL;
	cxt.ckconstraints = NIL;
	cxt.fkconstraints = NIL;
	cxt.ixconstraints = NIL;
	cxt.inh_indexes = NIL;
	cxt.blist = NIL;
	cxt.alist = NIL;
	cxt.pkey = NULL;

	/*
	 * The only subtypes that currently require parse transformation handling
	 * are ADD COLUMN and ADD CONSTRAINT.  These largely re-use code from
	 * CREATE TABLE.
	 */
	foreach(lcmd, stmt->cmds)
	{
		AlterTableCmd *cmd = (AlterTableCmd *) lfirst(lcmd);

		switch (cmd->subtype)
		{
			case AT_AddColumn:
			case AT_AddColumnToView:
				{
					ColumnDef  *def = (ColumnDef *) cmd->def;

					Assert(IsA(def, ColumnDef));
					transformColumnDefinition(&cxt, def);

					/*
					 * If the column has a non-null default, we can't skip
					 * validation of foreign keys.
					 */
					if (def->raw_default != NULL)
						skipValidation = false;

					/*
					 * All constraints are processed in other ways. Remove the
					 * original list
					 */
					def->constraints = NIL;

					newcmds = lappend(newcmds, cmd);
					break;
				}
			case AT_AddConstraint:

				/*
				 * The original AddConstraint cmd node doesn't go to newcmds
				 */
				if (IsA(cmd->def, Constraint))
				{
					transformTableConstraint(&cxt, (Constraint *) cmd->def);
					if (((Constraint *) cmd->def)->contype == CONSTR_FOREIGN)
						skipValidation = false;
				}
				else
					elog(ERROR, "unrecognized node type: %d",
						 (int) nodeTag(cmd->def));
				break;

			case AT_ProcessedConstraint:

				/*
				 * Already-transformed ADD CONSTRAINT, so just make it look
				 * like the standard case.
				 */
				cmd->subtype = AT_AddConstraint;
				newcmds = lappend(newcmds, cmd);
				break;

			default:
				newcmds = lappend(newcmds, cmd);
				break;
		}
	}

	/*
	 * transformIndexConstraints wants cxt.alist to contain only index
	 * statements, so transfer anything we already have into save_alist
	 * immediately.
	 */
	save_alist = cxt.alist;
	cxt.alist = NIL;

	/* Postprocess index and FK constraints */
	transformIndexConstraints(&cxt);

	transformFKConstraints(&cxt, skipValidation, true);

	/*
	 * Push any index-creation commands into the ALTER, so that they can be
	 * scheduled nicely by tablecmds.c.  Note that tablecmds.c assumes that
	 * the IndexStmt attached to an AT_AddIndex or AT_AddIndexConstraint
	 * subcommand has already been through transformIndexStmt.
	 */
	foreach(l, cxt.alist)
	{
		IndexStmt  *idxstmt = (IndexStmt *) lfirst(l);

		Assert(IsA(idxstmt, IndexStmt));
		idxstmt = transformIndexStmt(idxstmt, queryString);
		newcmd = makeNode(AlterTableCmd);
		newcmd->subtype = OidIsValid(idxstmt->indexOid) ? AT_AddIndexConstraint : AT_AddIndex;
		newcmd->def = (Node *) idxstmt;
		newcmds = lappend(newcmds, newcmd);
	}
	cxt.alist = NIL;

	/* Append any CHECK or FK constraints to the commands list */
	foreach(l, cxt.ckconstraints)
	{
		newcmd = makeNode(AlterTableCmd);
		newcmd->subtype = AT_AddConstraint;
		newcmd->def = (Node *) lfirst(l);
		newcmds = lappend(newcmds, newcmd);
	}
	foreach(l, cxt.fkconstraints)
	{
		newcmd = makeNode(AlterTableCmd);
		newcmd->subtype = AT_AddConstraint;
		newcmd->def = (Node *) lfirst(l);
		newcmds = lappend(newcmds, newcmd);
	}

	/* Close rel but keep lock */
	relation_close(rel, NoLock);

	/*
	 * Output results.
	 */
	stmt->cmds = newcmds;

	result = lappend(cxt.blist, stmt);
	result = list_concat(result, cxt.alist);
	result = list_concat(result, save_alist);

	return result;
}