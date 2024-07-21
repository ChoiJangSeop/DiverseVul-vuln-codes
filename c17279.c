Event_db_repository::open_event_table(THD *thd, enum thr_lock_type lock_type,
                                      TABLE **table)
{
  TABLE_LIST tables;
  DBUG_ENTER("Event_db_repository::open_event_table");

  tables.init_one_table(&MYSQL_SCHEMA_NAME, &MYSQL_EVENT_NAME, 0, lock_type);

  if (open_and_lock_tables(thd, &tables, FALSE, MYSQL_LOCK_IGNORE_TIMEOUT))
    DBUG_RETURN(TRUE);

  *table= tables.table;
  tables.table->use_all_columns();

  if (table_intact.check(*table, &event_table_def))
  {
    close_thread_tables(thd);
    my_error(ER_EVENT_OPEN_TABLE_FAILED, MYF(0));
    DBUG_RETURN(TRUE);
  }

  DBUG_RETURN(FALSE);
}