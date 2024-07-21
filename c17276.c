void close_thread_table(THD *thd, TABLE **table_ptr)
{
  TABLE *table= *table_ptr;
  DBUG_ENTER("close_thread_table");
  DBUG_PRINT("tcache", ("table: '%s'.'%s' %p", table->s->db.str,
                        table->s->table_name.str, table));
  DBUG_ASSERT(!table->file->keyread_enabled());
  DBUG_ASSERT(!table->file || table->file->inited == handler::NONE);

  /*
    The metadata lock must be released after giving back
    the table to the table cache.
  */
  DBUG_ASSERT(thd->mdl_context.is_lock_owner(MDL_key::TABLE,
                                             table->s->db.str,
                                             table->s->table_name.str,
                                             MDL_SHARED));
  table->mdl_ticket= NULL;

  if (table->file)
  {
    table->file->update_global_table_stats();
    table->file->update_global_index_stats();
  }

  /*
    This look is needed to allow THD::notify_shared_lock() to
    traverse the thd->open_tables list without having to worry that
    some of the tables are removed from under it
  */

  mysql_mutex_lock(&thd->LOCK_thd_data);
  *table_ptr=table->next;
  mysql_mutex_unlock(&thd->LOCK_thd_data);

  if (! table->needs_reopen())
  {
    /* Avoid having MERGE tables with attached children in table cache. */
    table->file->extra(HA_EXTRA_DETACH_CHILDREN);
    /* Free memory and reset for next loop. */
    free_field_buffers_larger_than(table, MAX_TDC_BLOB_SIZE);
    table->file->ha_reset();
  }

  /*
    Do this *before* entering the TABLE_SHARE::tdc.LOCK_table_share
    critical section.
  */
  MYSQL_UNBIND_TABLE(table->file);

  tc_release_table(table);
  DBUG_VOID_RETURN;
}