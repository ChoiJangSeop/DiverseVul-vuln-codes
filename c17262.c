static bool fix_all_session_vcol_exprs(THD *thd, TABLE_LIST *tables)
{
  Security_context *save_security_ctx= thd->security_ctx;
  TABLE_LIST *first_not_own= thd->lex->first_not_own_table();
  DBUG_ENTER("fix_session_vcol_expr");

  int error= 0;
  for (TABLE_LIST *table= tables; table && table != first_not_own && !error;
       table= table->next_global)
  {
    TABLE *t= table->table;
    if (!table->placeholder() && t->s->vcols_need_refixing &&
         table->lock_type >= TL_WRITE_ALLOW_WRITE)
    {
      Query_arena *stmt_backup= thd->stmt_arena;
      if (thd->stmt_arena->is_conventional())
        thd->stmt_arena= t->expr_arena;
      if (table->security_ctx)
        thd->security_ctx= table->security_ctx;

      error= t->fix_vcol_exprs(thd);

      thd->security_ctx= save_security_ctx;
      thd->stmt_arena= stmt_backup;
    }
  }
  DBUG_RETURN(error);
}