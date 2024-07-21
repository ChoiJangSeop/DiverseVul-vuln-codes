int setup_order(THD *thd, Ref_ptr_array ref_pointer_array, TABLE_LIST *tables,
                List<Item> &fields, List<Item> &all_fields, ORDER *order,
                bool from_window_spec)
{ 
  SELECT_LEX *select = thd->lex->current_select;
  enum_parsing_place context_analysis_place=
                     thd->lex->current_select->context_analysis_place;
  thd->where="order clause";
  const bool for_union= select->master_unit()->is_unit_op() &&
    select == select->master_unit()->fake_select_lex;
  for (uint number = 1; order; order=order->next, number++)
  {
    if (find_order_in_list(thd, ref_pointer_array, tables, order, fields,
                           all_fields, false, true, from_window_spec))
      return 1;
    Item * const item= *order->item;
    if (item->with_window_func && context_analysis_place != IN_ORDER_BY)
    {
      my_error(ER_WINDOW_FUNCTION_IN_WINDOW_SPEC, MYF(0));
      return 1;
    }

    /*
      UNION queries cannot be used with an aggregate function in
      an ORDER BY clause
    */

    if (for_union && (item->with_sum_func() || item->with_window_func))
    {
      my_error(ER_AGGREGATE_ORDER_FOR_UNION, MYF(0), number);
      return 1;
    }

    if (from_window_spec && item->with_sum_func() &&
        item->type() != Item::SUM_FUNC_ITEM)
    {
      item->split_sum_func(thd, ref_pointer_array,
                           all_fields, SPLIT_SUM_SELECT);
    }
  }
  return 0;
}