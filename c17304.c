bool Item_sum_avg::fix_length_and_dec()
{
  if (Item_sum_sum::fix_length_and_dec())
    return TRUE;
  maybe_null=null_value=1;
  prec_increment= current_thd->variables.div_precincrement;
  if (Item_sum_avg::result_type() == DECIMAL_RESULT)
  {
    int precision= args[0]->decimal_precision() + prec_increment;
    decimals= MY_MIN(args[0]->decimals + prec_increment, DECIMAL_MAX_SCALE);
    max_length= my_decimal_precision_to_length_no_truncation(precision,
                                                             decimals,
                                                             unsigned_flag);
    f_precision= MY_MIN(precision+DECIMAL_LONGLONG_DIGITS, DECIMAL_MAX_PRECISION);
    f_scale=  args[0]->decimals;
    dec_bin_size= my_decimal_get_binary_size(f_precision, f_scale);
  }
  else
  {
    decimals= MY_MIN(args[0]->decimals + prec_increment,
                     FLOATING_POINT_DECIMALS);
    max_length= MY_MIN(args[0]->max_length + prec_increment, float_length(decimals));
  }
  return FALSE;
}