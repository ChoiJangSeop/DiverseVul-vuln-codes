bool Item_func_round::fix_length_and_dec()
{
  int      decimals_to_set;
  longlong val1;
  bool     val1_unsigned;
  
  unsigned_flag= args[0]->unsigned_flag;
  if (!args[1]->const_item())
  {
    decimals= args[0]->decimals;
    max_length= float_length(decimals);
    if (args[0]->result_type() == DECIMAL_RESULT)
    {
      max_length++;
      set_handler_by_result_type(DECIMAL_RESULT);
    }
    else
      set_handler_by_result_type(REAL_RESULT);
    return FALSE;
  }

  val1= args[1]->val_int();
  if ((null_value= args[1]->null_value))
    return FALSE;

  val1_unsigned= args[1]->unsigned_flag;
  if (val1 < 0)
    decimals_to_set= val1_unsigned ? INT_MAX : 0;
  else
    decimals_to_set= (val1 > INT_MAX) ? INT_MAX : (int) val1;

  if (args[0]->decimals == NOT_FIXED_DEC)
  {
    decimals= MY_MIN(decimals_to_set, NOT_FIXED_DEC);
    max_length= float_length(decimals);
    set_handler_by_result_type(REAL_RESULT);
    return FALSE;
  }
  
  switch (args[0]->result_type()) {
  case REAL_RESULT:
  case STRING_RESULT:
    set_handler_by_result_type(REAL_RESULT);
    decimals= MY_MIN(decimals_to_set, NOT_FIXED_DEC);
    max_length= float_length(decimals);
    break;
  case INT_RESULT:
    if ((!decimals_to_set && truncate) || (args[0]->decimal_precision() < DECIMAL_LONGLONG_DIGITS))
    {
      int length_can_increase= MY_TEST(!truncate && (val1 < 0) &&
                                       !val1_unsigned);
      max_length= args[0]->max_length + length_can_increase;
      /* Here we can keep INT_RESULT */
      set_handler_by_result_type(INT_RESULT);
      decimals= 0;
      break;
    }
    /* fall through */
  case DECIMAL_RESULT:
  {
    set_handler_by_result_type(DECIMAL_RESULT);
    decimals_to_set= MY_MIN(DECIMAL_MAX_SCALE, decimals_to_set);
    int decimals_delta= args[0]->decimals - decimals_to_set;
    int precision= args[0]->decimal_precision();
    int length_increase= ((decimals_delta <= 0) || truncate) ? 0:1;

    precision-= decimals_delta - length_increase;
    decimals= MY_MIN(decimals_to_set, DECIMAL_MAX_SCALE);
    if (!precision)
      precision= 1; // DECIMAL(0,0) -> DECIMAL(1,0)
    max_length= my_decimal_precision_to_length_no_truncation(precision,
                                                             decimals,
                                                             unsigned_flag);
    break;
  }
  case ROW_RESULT:
  case TIME_RESULT:
    DBUG_ASSERT(0); /* This result type isn't handled */
  }
  return FALSE;
}