Type_handler_decimal_result::make_num_distinct_aggregator_field(
                                                            MEM_ROOT *mem_root,
                                                            const Item *item)
                                                            const
{
  DBUG_ASSERT(item->decimals <= DECIMAL_MAX_SCALE);
  return new (mem_root)
         Field_new_decimal(NULL, item->max_length,
                           (uchar *) (item->maybe_null ? "" : 0),
                           item->maybe_null ? 1 : 0, Field::NONE,
                           item->name, item->decimals, 0, item->unsigned_flag);
}