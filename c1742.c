ProcessUtilitySlow(Node *parsetree,
				   const char *queryString,
				   ProcessUtilityContext context,
				   ParamListInfo params,
				   DestReceiver *dest,
				   char *completionTag)
{
	bool		isTopLevel = (context == PROCESS_UTILITY_TOPLEVEL);
	bool		isCompleteQuery = (context <= PROCESS_UTILITY_QUERY);
	bool		needCleanup;

	/* All event trigger calls are done only when isCompleteQuery is true */
	needCleanup = isCompleteQuery && EventTriggerBeginCompleteQuery();

	/* PG_TRY block is to ensure we call EventTriggerEndCompleteQuery */
	PG_TRY();
	{
		if (isCompleteQuery)
			EventTriggerDDLCommandStart(parsetree);

		switch (nodeTag(parsetree))
		{
				/*
				 * relation and attribute manipulation
				 */
			case T_CreateSchemaStmt:
				CreateSchemaCommand((CreateSchemaStmt *) parsetree,
									queryString);
				break;

			case T_CreateStmt:
			case T_CreateForeignTableStmt:
				{
					List	   *stmts;
					ListCell   *l;
					Oid			relOid;

					/* Run parse analysis ... */
					stmts = transformCreateStmt((CreateStmt *) parsetree,
												queryString);

					/* ... and do it */
					foreach(l, stmts)
					{
						Node	   *stmt = (Node *) lfirst(l);

						if (IsA(stmt, CreateStmt))
						{
							Datum		toast_options;
							static char *validnsps[] = HEAP_RELOPT_NAMESPACES;

							/* Create the table itself */
							relOid = DefineRelation((CreateStmt *) stmt,
													RELKIND_RELATION,
													InvalidOid);

							/*
							 * Let AlterTableCreateToastTable decide if this
							 * one needs a secondary relation too.
							 */
							CommandCounterIncrement();

							/*
							 * parse and validate reloptions for the toast
							 * table
							 */
							toast_options = transformRelOptions((Datum) 0,
											  ((CreateStmt *) stmt)->options,
																"toast",
																validnsps,
																true,
																false);
							(void) heap_reloptions(RELKIND_TOASTVALUE,
												   toast_options,
												   true);

							AlterTableCreateToastTable(relOid, toast_options);
						}
						else if (IsA(stmt, CreateForeignTableStmt))
						{
							/* Create the table itself */
							relOid = DefineRelation((CreateStmt *) stmt,
													RELKIND_FOREIGN_TABLE,
													InvalidOid);
							CreateForeignTable((CreateForeignTableStmt *) stmt,
											   relOid);
						}
						else
						{
							/* Recurse for anything else */
							ProcessUtility(stmt,
										   queryString,
										   PROCESS_UTILITY_SUBCOMMAND,
										   params,
										   None_Receiver,
										   NULL);
						}

						/* Need CCI between commands */
						if (lnext(l) != NULL)
							CommandCounterIncrement();
					}
				}
				break;

			case T_AlterTableStmt:
				{
					AlterTableStmt *atstmt = (AlterTableStmt *) parsetree;
					Oid			relid;
					List	   *stmts;
					ListCell   *l;
					LOCKMODE	lockmode;

					/*
					 * Figure out lock mode, and acquire lock.	This also does
					 * basic permissions checks, so that we won't wait for a
					 * lock on (for example) a relation on which we have no
					 * permissions.
					 */
					lockmode = AlterTableGetLockLevel(atstmt->cmds);
					relid = AlterTableLookupRelation(atstmt, lockmode);

					if (OidIsValid(relid))
					{
						/* Run parse analysis ... */
						stmts = transformAlterTableStmt(atstmt, queryString);

						/* ... and do it */
						foreach(l, stmts)
						{
							Node	   *stmt = (Node *) lfirst(l);

							if (IsA(stmt, AlterTableStmt))
							{
								/* Do the table alteration proper */
								AlterTable(relid, lockmode,
										   (AlterTableStmt *) stmt);
							}
							else
							{
								/* Recurse for anything else */
								ProcessUtility(stmt,
											   queryString,
											   PROCESS_UTILITY_SUBCOMMAND,
											   params,
											   None_Receiver,
											   NULL);
							}

							/* Need CCI between commands */
							if (lnext(l) != NULL)
								CommandCounterIncrement();
						}
					}
					else
						ereport(NOTICE,
						  (errmsg("relation \"%s\" does not exist, skipping",
								  atstmt->relation->relname)));
				}
				break;

			case T_AlterDomainStmt:
				{
					AlterDomainStmt *stmt = (AlterDomainStmt *) parsetree;

					/*
					 * Some or all of these functions are recursive to cover
					 * inherited things, so permission checks are done there.
					 */
					switch (stmt->subtype)
					{
						case 'T':		/* ALTER DOMAIN DEFAULT */

							/*
							 * Recursively alter column default for table and,
							 * if requested, for descendants
							 */
							AlterDomainDefault(stmt->typeName,
											   stmt->def);
							break;
						case 'N':		/* ALTER DOMAIN DROP NOT NULL */
							AlterDomainNotNull(stmt->typeName,
											   false);
							break;
						case 'O':		/* ALTER DOMAIN SET NOT NULL */
							AlterDomainNotNull(stmt->typeName,
											   true);
							break;
						case 'C':		/* ADD CONSTRAINT */
							AlterDomainAddConstraint(stmt->typeName,
													 stmt->def);
							break;
						case 'X':		/* DROP CONSTRAINT */
							AlterDomainDropConstraint(stmt->typeName,
													  stmt->name,
													  stmt->behavior,
													  stmt->missing_ok);
							break;
						case 'V':		/* VALIDATE CONSTRAINT */
							AlterDomainValidateConstraint(stmt->typeName,
														  stmt->name);
							break;
						default:		/* oops */
							elog(ERROR, "unrecognized alter domain type: %d",
								 (int) stmt->subtype);
							break;
					}
				}
				break;

				/*
				 * ************* object creation / destruction **************
				 */
			case T_DefineStmt:
				{
					DefineStmt *stmt = (DefineStmt *) parsetree;

					switch (stmt->kind)
					{
						case OBJECT_AGGREGATE:
							DefineAggregate(stmt->defnames, stmt->args,
											stmt->oldstyle, stmt->definition,
											queryString);
							break;
						case OBJECT_OPERATOR:
							Assert(stmt->args == NIL);
							DefineOperator(stmt->defnames, stmt->definition);
							break;
						case OBJECT_TYPE:
							Assert(stmt->args == NIL);
							DefineType(stmt->defnames, stmt->definition);
							break;
						case OBJECT_TSPARSER:
							Assert(stmt->args == NIL);
							DefineTSParser(stmt->defnames, stmt->definition);
							break;
						case OBJECT_TSDICTIONARY:
							Assert(stmt->args == NIL);
							DefineTSDictionary(stmt->defnames,
											   stmt->definition);
							break;
						case OBJECT_TSTEMPLATE:
							Assert(stmt->args == NIL);
							DefineTSTemplate(stmt->defnames,
											 stmt->definition);
							break;
						case OBJECT_TSCONFIGURATION:
							Assert(stmt->args == NIL);
							DefineTSConfiguration(stmt->defnames,
												  stmt->definition);
							break;
						case OBJECT_COLLATION:
							Assert(stmt->args == NIL);
							DefineCollation(stmt->defnames, stmt->definition);
							break;
						default:
							elog(ERROR, "unrecognized define stmt type: %d",
								 (int) stmt->kind);
							break;
					}
				}
				break;

			case T_IndexStmt:	/* CREATE INDEX */
				{
					IndexStmt  *stmt = (IndexStmt *) parsetree;

					if (stmt->concurrent)
						PreventTransactionChain(isTopLevel,
												"CREATE INDEX CONCURRENTLY");

					CheckRelationOwnership(stmt->relation, true);

					/* Run parse analysis ... */
					stmt = transformIndexStmt(stmt, queryString);

					/* ... and do it */
					DefineIndex(stmt,
								InvalidOid,		/* no predefined OID */
								false,	/* is_alter_table */
								true,	/* check_rights */
								false,	/* skip_build */
								false); /* quiet */
				}
				break;

			case T_CreateExtensionStmt:
				CreateExtension((CreateExtensionStmt *) parsetree);
				break;

			case T_AlterExtensionStmt:
				ExecAlterExtensionStmt((AlterExtensionStmt *) parsetree);
				break;

			case T_AlterExtensionContentsStmt:
				ExecAlterExtensionContentsStmt((AlterExtensionContentsStmt *) parsetree);
				break;

			case T_CreateFdwStmt:
				CreateForeignDataWrapper((CreateFdwStmt *) parsetree);
				break;

			case T_AlterFdwStmt:
				AlterForeignDataWrapper((AlterFdwStmt *) parsetree);
				break;

			case T_CreateForeignServerStmt:
				CreateForeignServer((CreateForeignServerStmt *) parsetree);
				break;

			case T_AlterForeignServerStmt:
				AlterForeignServer((AlterForeignServerStmt *) parsetree);
				break;

			case T_CreateUserMappingStmt:
				CreateUserMapping((CreateUserMappingStmt *) parsetree);
				break;

			case T_AlterUserMappingStmt:
				AlterUserMapping((AlterUserMappingStmt *) parsetree);
				break;

			case T_DropUserMappingStmt:
				RemoveUserMapping((DropUserMappingStmt *) parsetree);
				break;

			case T_CompositeTypeStmt:	/* CREATE TYPE (composite) */
				{
					CompositeTypeStmt *stmt = (CompositeTypeStmt *) parsetree;

					DefineCompositeType(stmt->typevar, stmt->coldeflist);
				}
				break;

			case T_CreateEnumStmt:		/* CREATE TYPE AS ENUM */
				DefineEnum((CreateEnumStmt *) parsetree);
				break;

			case T_CreateRangeStmt:		/* CREATE TYPE AS RANGE */
				DefineRange((CreateRangeStmt *) parsetree);
				break;

			case T_AlterEnumStmt:		/* ALTER TYPE (enum) */
				AlterEnum((AlterEnumStmt *) parsetree, isTopLevel);
				break;

			case T_ViewStmt:	/* CREATE VIEW */
				DefineView((ViewStmt *) parsetree, queryString);
				break;

			case T_CreateFunctionStmt:	/* CREATE FUNCTION */
				CreateFunction((CreateFunctionStmt *) parsetree, queryString);
				break;

			case T_AlterFunctionStmt:	/* ALTER FUNCTION */
				AlterFunction((AlterFunctionStmt *) parsetree);
				break;

			case T_RuleStmt:	/* CREATE RULE */
				DefineRule((RuleStmt *) parsetree, queryString);
				break;

			case T_CreateSeqStmt:
				DefineSequence((CreateSeqStmt *) parsetree);
				break;

			case T_AlterSeqStmt:
				AlterSequence((AlterSeqStmt *) parsetree);
				break;

			case T_CreateTableAsStmt:
				ExecCreateTableAs((CreateTableAsStmt *) parsetree,
								  queryString, params, completionTag);
				break;

			case T_RefreshMatViewStmt:
				ExecRefreshMatView((RefreshMatViewStmt *) parsetree,
								   queryString, params, completionTag);
				break;

			case T_CreateTrigStmt:
				(void) CreateTrigger((CreateTrigStmt *) parsetree, queryString,
									 InvalidOid, InvalidOid, false);
				break;

			case T_CreatePLangStmt:
				CreateProceduralLanguage((CreatePLangStmt *) parsetree);
				break;

			case T_CreateDomainStmt:
				DefineDomain((CreateDomainStmt *) parsetree);
				break;

			case T_CreateConversionStmt:
				CreateConversionCommand((CreateConversionStmt *) parsetree);
				break;

			case T_CreateCastStmt:
				CreateCast((CreateCastStmt *) parsetree);
				break;

			case T_CreateOpClassStmt:
				DefineOpClass((CreateOpClassStmt *) parsetree);
				break;

			case T_CreateOpFamilyStmt:
				DefineOpFamily((CreateOpFamilyStmt *) parsetree);
				break;

			case T_AlterOpFamilyStmt:
				AlterOpFamily((AlterOpFamilyStmt *) parsetree);
				break;

			case T_AlterTSDictionaryStmt:
				AlterTSDictionary((AlterTSDictionaryStmt *) parsetree);
				break;

			case T_AlterTSConfigurationStmt:
				AlterTSConfiguration((AlterTSConfigurationStmt *) parsetree);
				break;

			case T_DropStmt:
				ExecDropStmt((DropStmt *) parsetree, isTopLevel);
				break;

			case T_RenameStmt:
				ExecRenameStmt((RenameStmt *) parsetree);
				break;

			case T_AlterObjectSchemaStmt:
				ExecAlterObjectSchemaStmt((AlterObjectSchemaStmt *) parsetree);
				break;

			case T_AlterOwnerStmt:
				ExecAlterOwnerStmt((AlterOwnerStmt *) parsetree);
				break;

			case T_DropOwnedStmt:
				DropOwnedObjects((DropOwnedStmt *) parsetree);
				break;

			case T_AlterDefaultPrivilegesStmt:
				ExecAlterDefaultPrivilegesStmt((AlterDefaultPrivilegesStmt *) parsetree);
				break;

			default:
				elog(ERROR, "unrecognized node type: %d",
					 (int) nodeTag(parsetree));
				break;
		}

		if (isCompleteQuery)
		{
			EventTriggerSQLDrop(parsetree);
			EventTriggerDDLCommandEnd(parsetree);
		}
	}
	PG_CATCH();
	{
		if (needCleanup)
			EventTriggerEndCompleteQuery();
		PG_RE_THROW();
	}
	PG_END_TRY();

	if (needCleanup)
		EventTriggerEndCompleteQuery();
}