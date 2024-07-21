bool fix_session_vcol_expr(THD *thd, Virtual_column_info *vcol)
{
  DBUG_ENTER("fix_session_vcol_expr");
  if (!(vcol->flags & (VCOL_TIME_FUNC|VCOL_SESSION_FUNC)))
    DBUG_RETURN(0);

  vcol->expr->walk(&Item::cleanup_excluding_fields_processor, 0, 0);
  DBUG_ASSERT(!vcol->expr->fixed);
  DBUG_RETURN(fix_vcol_expr(thd, vcol));
}