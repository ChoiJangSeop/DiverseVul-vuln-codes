fmgr_sql_validator(PG_FUNCTION_ARGS)
{
	Oid			funcoid = PG_GETARG_OID(0);
	HeapTuple	tuple;
	Form_pg_proc proc;
	List	   *raw_parsetree_list;
	List	   *querytree_list;
	ListCell   *lc;
	bool		isnull;
	Datum		tmp;
	char	   *prosrc;
	parse_error_callback_arg callback_arg;
	ErrorContextCallback sqlerrcontext;
	bool		haspolyarg;
	int			i;

	tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(funcoid));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for function %u", funcoid);
	proc = (Form_pg_proc) GETSTRUCT(tuple);

	/* Disallow pseudotype result */
	/* except for RECORD, VOID, or polymorphic */
	if (get_typtype(proc->prorettype) == TYPTYPE_PSEUDO &&
		proc->prorettype != RECORDOID &&
		proc->prorettype != VOIDOID &&
		!IsPolymorphicType(proc->prorettype))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
				 errmsg("SQL functions cannot return type %s",
						format_type_be(proc->prorettype))));

	/* Disallow pseudotypes in arguments */
	/* except for polymorphic */
	haspolyarg = false;
	for (i = 0; i < proc->pronargs; i++)
	{
		if (get_typtype(proc->proargtypes.values[i]) == TYPTYPE_PSEUDO)
		{
			if (IsPolymorphicType(proc->proargtypes.values[i]))
				haspolyarg = true;
			else
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
					 errmsg("SQL functions cannot have arguments of type %s",
							format_type_be(proc->proargtypes.values[i]))));
		}
	}

	/* Postpone body checks if !check_function_bodies */
	if (check_function_bodies)
	{
		tmp = SysCacheGetAttr(PROCOID, tuple, Anum_pg_proc_prosrc, &isnull);
		if (isnull)
			elog(ERROR, "null prosrc");

		prosrc = TextDatumGetCString(tmp);

		/*
		 * Setup error traceback support for ereport().
		 */
		callback_arg.proname = NameStr(proc->proname);
		callback_arg.prosrc = prosrc;

		sqlerrcontext.callback = sql_function_parse_error_callback;
		sqlerrcontext.arg = (void *) &callback_arg;
		sqlerrcontext.previous = error_context_stack;
		error_context_stack = &sqlerrcontext;

		/*
		 * We can't do full prechecking of the function definition if there
		 * are any polymorphic input types, because actual datatypes of
		 * expression results will be unresolvable.  The check will be done at
		 * runtime instead.
		 *
		 * We can run the text through the raw parser though; this will at
		 * least catch silly syntactic errors.
		 */
		raw_parsetree_list = pg_parse_query(prosrc);

		if (!haspolyarg)
		{
			/*
			 * OK to do full precheck: analyze and rewrite the queries, then
			 * verify the result type.
			 */
			SQLFunctionParseInfoPtr pinfo;

			/* But first, set up parameter information */
			pinfo = prepare_sql_fn_parse_info(tuple, NULL, InvalidOid);

			querytree_list = NIL;
			foreach(lc, raw_parsetree_list)
			{
				Node	   *parsetree = (Node *) lfirst(lc);
				List	   *querytree_sublist;

				querytree_sublist = pg_analyze_and_rewrite_params(parsetree,
																  prosrc,
									   (ParserSetupHook) sql_fn_parser_setup,
																  pinfo);
				querytree_list = list_concat(querytree_list,
											 querytree_sublist);
			}

			(void) check_sql_fn_retval(funcoid, proc->prorettype,
									   querytree_list,
									   NULL, NULL);
		}

		error_context_stack = sqlerrcontext.previous;
	}

	ReleaseSysCache(tuple);

	PG_RETURN_VOID();
}