void close_thread_tables(THD *thd)
{
  TABLE *table;
  DBUG_ENTER("close_thread_tables");

  THD_STAGE_INFO(thd, stage_closing_tables);

#ifdef EXTRA_DEBUG
  DBUG_PRINT("tcache", ("open tables:"));
  for (table= thd->open_tables; table; table= table->next)
    DBUG_PRINT("tcache", ("table: '%s'.'%s' %p", table->s->db.str,
                          table->s->table_name.str, table));
#endif

#if defined(ENABLED_DEBUG_SYNC)
  /* debug_sync may not be initialized for some slave threads */
  if (thd->debug_sync_control)
    DEBUG_SYNC(thd, "before_close_thread_tables");
#endif

  DBUG_ASSERT(thd->transaction.stmt.is_empty() || thd->in_sub_stmt ||
              (thd->state_flags & Open_tables_state::BACKUPS_AVAIL));

  /* Detach MERGE children after every statement. Even under LOCK TABLES. */
  for (table= thd->open_tables; table; table= table->next)
  {
    /* Table might be in use by some outer statement. */
    DBUG_PRINT("tcache", ("table: '%s'  query_id: %lu",
                          table->s->table_name.str, (ulong) table->query_id));
    if (thd->locked_tables_mode <= LTM_LOCK_TABLES ||
        table->query_id == thd->query_id)
    {
      DBUG_ASSERT(table->file);
      table->file->extra(HA_EXTRA_DETACH_CHILDREN);
    }
  }

  /*
    We are assuming here that thd->derived_tables contains ONLY derived
    tables for this substatement. i.e. instead of approach which uses
    query_id matching for determining which of the derived tables belong
    to this substatement we rely on the ability of substatements to
    save/restore thd->derived_tables during their execution.

    TODO: Probably even better approach is to simply associate list of
          derived tables with (sub-)statement instead of thread and destroy
          them at the end of its execution.
  */
  if (thd->derived_tables)
  {
    TABLE *next;
    /*
      Close all derived tables generated in queries like
      SELECT * FROM (SELECT * FROM t1)
    */
    for (table= thd->derived_tables ; table ; table= next)
    {
      next= table->next;
      free_tmp_table(thd, table);
    }
    thd->derived_tables= 0;
  }

  if (thd->rec_tables)
  {
    TABLE *next;
    /*
      Close all temporary tables created for recursive table references.
      This action was postponed because the table could be used in the
      statements like  ANALYZE WITH r AS (...) SELECT * from r
      where r is defined through recursion. 
    */
    for (table= thd->rec_tables ; table ; table= next)
    {
      next= table->next;
      free_tmp_table(thd, table);
    }
    thd->rec_tables= 0;
  }

  /*
    Mark all temporary tables used by this statement as free for reuse.
  */
  thd->mark_tmp_tables_as_free_for_reuse();

  if (thd->locked_tables_mode)
  {

    /* Ensure we are calling ha_reset() for all used tables */
    mark_used_tables_as_free_for_reuse(thd, thd->open_tables);

    /*
      We are under simple LOCK TABLES or we're inside a sub-statement
      of a prelocked statement, so should not do anything else.

      Note that even if we are in LTM_LOCK_TABLES mode and statement
      requires prelocking (e.g. when we are closing tables after
      failing ot "open" all tables required for statement execution)
      we will exit this function a few lines below.
    */
    if (! thd->lex->requires_prelocking())
      DBUG_VOID_RETURN;

    /*
      We are in the top-level statement of a prelocked statement,
      so we have to leave the prelocked mode now with doing implicit
      UNLOCK TABLES if needed.
    */
    if (thd->locked_tables_mode == LTM_PRELOCKED_UNDER_LOCK_TABLES)
      thd->locked_tables_mode= LTM_LOCK_TABLES;

    if (thd->locked_tables_mode == LTM_LOCK_TABLES)
      DBUG_VOID_RETURN;

    thd->leave_locked_tables_mode();

    /* Fallthrough */
  }

  if (thd->lock)
  {
    /*
      For RBR we flush the pending event just before we unlock all the
      tables.  This means that we are at the end of a topmost
      statement, so we ensure that the STMT_END_F flag is set on the
      pending event.  For statements that are *inside* stored
      functions, the pending event will not be flushed: that will be
      handled either before writing a query log event (inside
      binlog_query()) or when preparing a pending event.
     */
    (void)thd->binlog_flush_pending_rows_event(TRUE);
    mysql_unlock_tables(thd, thd->lock);
    thd->lock=0;
  }
  /*
    Closing a MERGE child before the parent would be fatal if the
    other thread tries to abort the MERGE lock in between.
  */
  while (thd->open_tables)
    (void) close_thread_table(thd, &thd->open_tables);

  DBUG_VOID_RETURN;
}