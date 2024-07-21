bool Virtual_column_info::fix_session_expr(THD *thd)
{
  DBUG_ENTER("fix_session_vcol_expr");
  if (!(flags & (VCOL_TIME_FUNC|VCOL_SESSION_FUNC)))
    DBUG_RETURN(0);

  expr->walk(&Item::cleanup_excluding_fields_processor, 0, 0);
  DBUG_ASSERT(!expr->fixed);
  DBUG_RETURN(fix_expr(thd));
}