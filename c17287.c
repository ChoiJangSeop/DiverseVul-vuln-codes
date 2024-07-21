bool Virtual_column_info::fix_and_check_expr(THD *thd, TABLE *table)
{
  DBUG_ENTER("fix_and_check_vcol_expr");
  DBUG_PRINT("info", ("vcol: %p", this));
  DBUG_ASSERT(expr);

  if (expr->fixed)
    DBUG_RETURN(0); // nothing to do

  if (fix_expr(thd))
    DBUG_RETURN(1);

  if (flags)
    DBUG_RETURN(0); // already checked, no need to do it again


  /* this was checked in check_expression(), but the frm could be mangled... */
  if (unlikely(expr->result_type() == ROW_RESULT))
  {
    my_error(ER_OPERAND_COLUMNS, MYF(0), 1);
    DBUG_RETURN(1);
  }

  /*
    Walk through the Item tree checking if all items are valid
    to be part of the virtual column
  */
  Item::vcol_func_processor_result res;
  res.errors= 0;

  int error= expr->walk(&Item::check_vcol_func_processor, 0, &res);
  if (unlikely(error || (res.errors & VCOL_IMPOSSIBLE)))
  {
    // this can only happen if the frm was corrupted
    my_error(ER_VIRTUAL_COLUMN_FUNCTION_IS_NOT_ALLOWED, MYF(0), res.name,
             get_vcol_type_name(), name.str);
    DBUG_RETURN(1);
  }
  else if (unlikely(res.errors & VCOL_AUTO_INC))
  {
    /*
      An auto_increment field may not be used in an expression for
      a check constraint, a default value or a generated column

      Note that this error condition is not detected during parsing
      of the statement because the field item does not have a field
      pointer at that time
    */
    myf warn= table->s->frm_version < FRM_VER_EXPRESSSIONS ? ME_JUST_WARNING : 0;
    my_error(ER_VIRTUAL_COLUMN_FUNCTION_IS_NOT_ALLOWED, MYF(warn),
             "AUTO_INCREMENT", get_vcol_type_name(), res.name);
    if (!warn)
      DBUG_RETURN(1);
  }
  flags= res.errors;

  if (flags & VCOL_SESSION_FUNC)
    table->s->vcols_need_refixing= true;

  DBUG_RETURN(0);
}