bool Item_sum_sum::fix_length_and_dec()
{
  DBUG_ENTER("Item_sum_sum::fix_length_and_dec");
  maybe_null=null_value=1;
  decimals= args[0]->decimals;
  switch (args[0]->cast_to_int_type()) {
  case REAL_RESULT:
  case STRING_RESULT:
    set_handler_by_field_type(MYSQL_TYPE_DOUBLE);
    sum= 0.0;
    break;
  case INT_RESULT:
  case TIME_RESULT:
  case DECIMAL_RESULT:
  {
    /* SUM result can't be longer than length(arg) + length(MAX_ROWS) */
    int precision= args[0]->decimal_precision() + DECIMAL_LONGLONG_DIGITS;
    max_length= my_decimal_precision_to_length_no_truncation(precision,
                                                             decimals,
                                                             unsigned_flag);
    curr_dec_buff= 0;
    set_handler_by_field_type(MYSQL_TYPE_NEWDECIMAL);
    my_decimal_set_zero(dec_buffs);
    break;
  }
  case ROW_RESULT:
    DBUG_ASSERT(0);
  }
  DBUG_PRINT("info", ("Type: %s (%d, %d)",
                      (result_type() == REAL_RESULT ? "REAL_RESULT" :
                       result_type() == DECIMAL_RESULT ? "DECIMAL_RESULT" :
                       result_type() == INT_RESULT ? "INT_RESULT" :
                       "--ILLEGAL!!!--"),
                      max_length,
                      (int)decimals));
  DBUG_RETURN(FALSE);
}