mysql_select(THD *thd,
	     TABLE_LIST *tables, uint wild_num, List<Item> &fields,
	     COND *conds, uint og_num,  ORDER *order, ORDER *group,
	     Item *having, ORDER *proc_param, ulonglong select_options,
	     select_result *result, SELECT_LEX_UNIT *unit,
	     SELECT_LEX *select_lex)
{
  int err= 0;
  bool free_join= 1;
  DBUG_ENTER("mysql_select");

  select_lex->context.resolve_in_select_list= TRUE;
  JOIN *join;
  if (select_lex->join != 0)
  {
    join= select_lex->join;
    /*
      is it single SELECT in derived table, called in derived table
      creation
    */
    if (select_lex->linkage != DERIVED_TABLE_TYPE ||
	(select_options & SELECT_DESCRIBE))
    {
      if (select_lex->linkage != GLOBAL_OPTIONS_TYPE)
      {
        /*
          Original join tabs might be overwritten at first
          subselect execution. So we need to restore them.
        */
        Item_subselect *subselect= select_lex->master_unit()->item;
        if (subselect && subselect->is_uncacheable() && join->reinit())
          DBUG_RETURN(TRUE);
      }
      else
      {
        if ((err= join->prepare( tables, wild_num,
                                conds, og_num, order, false, group, having,
                                proc_param, select_lex, unit)))
	{
	  goto err;
	}
      }
    }
    free_join= 0;
    join->select_options= select_options;
  }
  else
  {
    if (thd->lex->describe)
      select_options|= SELECT_DESCRIBE;

    /*
      When in EXPLAIN, delay deleting the joins so that they are still
      available when we're producing EXPLAIN EXTENDED warning text.
    */
    if (select_options & SELECT_DESCRIBE)
      free_join= 0;

    if (!(join= new (thd->mem_root) JOIN(thd, fields, select_options, result)))
	DBUG_RETURN(TRUE);
    THD_STAGE_INFO(thd, stage_init);
    thd->lex->used_tables=0;
    if ((err= join->prepare(tables, wild_num,
                            conds, og_num, order, false, group, having, proc_param,
                            select_lex, unit)))
    {
      goto err;
    }
  }

  if ((err= join->optimize()))
  {
    goto err;					// 1
  }

  if (thd->lex->describe & DESCRIBE_EXTENDED)
  {
    join->conds_history= join->conds;
    join->having_history= (join->having?join->having:join->tmp_having);
  }

  if (unlikely(thd->is_error()))
    goto err;

  join->exec();

  if (thd->lex->describe & DESCRIBE_EXTENDED)
  {
    select_lex->where= join->conds_history;
    select_lex->having= join->having_history;
  }

err:
  if (free_join)
  {
    THD_STAGE_INFO(thd, stage_end);
    err|= (int)(select_lex->cleanup());
    DBUG_RETURN(err || thd->is_error());
  }
  DBUG_RETURN(join->error ? join->error: err);
}