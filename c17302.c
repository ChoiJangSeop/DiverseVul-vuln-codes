Field_new_decimal::Field_new_decimal(uchar *ptr_arg,
                                     uint32 len_arg, uchar *null_ptr_arg,
                                     uchar null_bit_arg,
                                     enum utype unireg_check_arg,
                                     const char *field_name_arg,
                                     uint8 dec_arg,bool zero_arg,
                                     bool unsigned_arg)
  :Field_num(ptr_arg, len_arg, null_ptr_arg, null_bit_arg,
             unireg_check_arg, field_name_arg, dec_arg, zero_arg, unsigned_arg)
{
  precision= my_decimal_length_to_precision(len_arg, dec_arg, unsigned_arg);
  set_if_smaller(precision, DECIMAL_MAX_PRECISION);
  DBUG_ASSERT((precision <= DECIMAL_MAX_PRECISION) &&
              (dec <= DECIMAL_MAX_SCALE));
  bin_size= my_decimal_get_binary_size(precision, dec);
}