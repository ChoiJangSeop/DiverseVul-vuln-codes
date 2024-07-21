bool lock_tables(THD *thd, TABLE_LIST *tables, uint count,
                 uint flags)
{
  TABLE_LIST *table;
  DBUG_ENTER("lock_tables");
  /*
    We can't meet statement requiring prelocking if we already
    in prelocked mode.
  */
  DBUG_ASSERT(thd->locked_tables_mode <= LTM_LOCK_TABLES ||
              !thd->lex->requires_prelocking());

  if (!tables && !thd->lex->requires_prelocking())
    DBUG_RETURN(thd->decide_logging_format(tables));

  /*
    Check for thd->locked_tables_mode to avoid a redundant
    and harmful attempt to lock the already locked tables again.
    Checking for thd->lock is not enough in some situations. For example,
    if a stored function contains
    "drop table t3; create temporary t3 ..; insert into t3 ...;"
    thd->lock may be 0 after drop tables, whereas locked_tables_mode
    is still on. In this situation an attempt to lock temporary
    table t3 will lead to a memory leak.
  */
  if (! thd->locked_tables_mode)
  {
    DBUG_ASSERT(thd->lock == 0);	// You must lock everything at once
    TABLE **start,**ptr;

    if (!(ptr=start=(TABLE**) thd->alloc(sizeof(TABLE*)*count)))
      DBUG_RETURN(TRUE);
    for (table= tables; table; table= table->next_global)
    {
      if (!table->placeholder())
	*(ptr++)= table->table;
    }

    DEBUG_SYNC(thd, "before_lock_tables_takes_lock");

    if (! (thd->lock= mysql_lock_tables(thd, start, (uint) (ptr - start),
                                        flags)))
      DBUG_RETURN(TRUE);

    DEBUG_SYNC(thd, "after_lock_tables_takes_lock");

    if (thd->lex->requires_prelocking() &&
        thd->lex->sql_command != SQLCOM_LOCK_TABLES)
    {
      TABLE_LIST *first_not_own= thd->lex->first_not_own_table();
      /*
        We just have done implicit LOCK TABLES, and now we have
        to emulate first open_and_lock_tables() after it.

        When open_and_lock_tables() is called for a single table out of
        a table list, the 'next_global' chain is temporarily broken. We
        may not find 'first_not_own' before the end of the "list".
        Look for example at those places where open_n_lock_single_table()
        is called. That function implements the temporary breaking of
        a table list for opening a single table.
      */
      for (table= tables;
           table && table != first_not_own;
           table= table->next_global)
      {
        if (!table->placeholder())
        {
          table->table->query_id= thd->query_id;
          if (check_lock_and_start_stmt(thd, thd->lex, table))
          {
            mysql_unlock_tables(thd, thd->lock);
            thd->lock= 0;
            DBUG_RETURN(TRUE);
          }
        }
      }
      /*
        Let us mark all tables which don't belong to the statement itself,
        and was marked as occupied during open_tables() as free for reuse.
      */
      mark_real_tables_as_free_for_reuse(first_not_own);
      DBUG_PRINT("info",("locked_tables_mode= LTM_PRELOCKED"));
      thd->enter_locked_tables_mode(LTM_PRELOCKED);
    }
  }
  else
  {
    TABLE_LIST *first_not_own= thd->lex->first_not_own_table();
    /*
      When open_and_lock_tables() is called for a single table out of
      a table list, the 'next_global' chain is temporarily broken. We
      may not find 'first_not_own' before the end of the "list".
      Look for example at those places where open_n_lock_single_table()
      is called. That function implements the temporary breaking of
      a table list for opening a single table.
    */
    for (table= tables;
         table && table != first_not_own;
         table= table->next_global)
    {
      if (table->placeholder())
        continue;

      /*
        In a stored function or trigger we should ensure that we won't change
        a table that is already used by the calling statement.
      */
      if (thd->locked_tables_mode >= LTM_PRELOCKED &&
          table->lock_type >= TL_WRITE_ALLOW_WRITE)
      {
        for (TABLE* opentab= thd->open_tables; opentab; opentab= opentab->next)
        {
          if (table->table->s == opentab->s && opentab->query_id &&
              table->table->query_id != opentab->query_id)
          {
            my_error(ER_CANT_UPDATE_USED_TABLE_IN_SF_OR_TRG, MYF(0),
                     table->table->s->table_name.str);
            DBUG_RETURN(TRUE);
          }
        }
      }

      if (check_lock_and_start_stmt(thd, thd->lex, table))
      {
	DBUG_RETURN(TRUE);
      }
    }
    /*
      If we are under explicit LOCK TABLES and our statement requires
      prelocking, we should mark all "additional" tables as free for use
      and enter prelocked mode.
    */
    if (thd->lex->requires_prelocking())
    {
      mark_real_tables_as_free_for_reuse(first_not_own);
      DBUG_PRINT("info",
                 ("thd->locked_tables_mode= LTM_PRELOCKED_UNDER_LOCK_TABLES"));
      thd->locked_tables_mode= LTM_PRELOCKED_UNDER_LOCK_TABLES;
    }
  }

  bool res= fix_all_session_vcol_exprs(thd, tables);
  if (!res)
    res= thd->decide_logging_format(tables);

  DBUG_RETURN(res);
}