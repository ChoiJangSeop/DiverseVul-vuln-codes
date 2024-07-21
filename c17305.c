Field_new_decimal::Field_new_decimal(uint32 len_arg,
                                     bool maybe_null_arg,
                                     const char *name,
                                     uint8 dec_arg,
                                     bool unsigned_arg)
  :Field_num((uchar*) 0, len_arg,
             maybe_null_arg ? (uchar*) "": 0, 0,
             NONE, name, dec_arg, 0, unsigned_arg)
{
  precision= my_decimal_length_to_precision(len_arg, dec_arg, unsigned_arg);
  set_if_smaller(precision, DECIMAL_MAX_PRECISION);
  DBUG_ASSERT((precision <= DECIMAL_MAX_PRECISION) &&
              (dec <= DECIMAL_MAX_SCALE));
  bin_size= my_decimal_get_binary_size(precision, dec);
}