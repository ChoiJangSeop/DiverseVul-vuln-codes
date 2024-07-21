static bool fix_vcol_expr(THD *thd, Virtual_column_info *vcol)
{
  DBUG_ENTER("fix_vcol_expr");

  const enum enum_column_usage saved_column_usage= thd->column_usage;
  thd->column_usage= COLUMNS_WRITE;

  int error= vcol->expr->fix_fields(thd, &vcol->expr);

  thd->column_usage= saved_column_usage;

  if (unlikely(error))
  {
    StringBuffer<MAX_FIELD_WIDTH> str;
    vcol->print(&str);
    my_error(ER_ERROR_EVALUATING_EXPRESSION, MYF(0), str.c_ptr_safe());
    DBUG_RETURN(1);
  }

  DBUG_RETURN(0);
}