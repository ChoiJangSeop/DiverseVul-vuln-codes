bool fix_session_vcol_expr_for_read(THD *thd, Field *field,
                                    Virtual_column_info *vcol)
{
  DBUG_ENTER("fix_session_vcol_expr_for_read");
  TABLE_LIST *tl= field->table->pos_in_table_list;
  if (!tl || tl->lock_type >= TL_WRITE_ALLOW_WRITE)
    DBUG_RETURN(0);
  Security_context *save_security_ctx= thd->security_ctx;
  if (tl->security_ctx)
    thd->security_ctx= tl->security_ctx;
  bool res= fix_session_vcol_expr(thd, vcol);
  thd->security_ctx= save_security_ctx;
  DBUG_RETURN(res);
}