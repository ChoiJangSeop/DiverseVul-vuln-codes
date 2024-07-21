bool st_select_lex::setup_ref_array(THD *thd, uint order_group_num)
{

  if (!((options & SELECT_DISTINCT) && !group_list.elements))
    hidden_bit_fields= 0;

  // find_order_in_list() may need some extra space, so multiply by two.
  order_group_num*= 2;

  /*
    We have to create array in prepared statement memory if it is a
    prepared statement
  */
  Query_arena *arena= thd->stmt_arena;
  const uint n_elems= (n_sum_items +
                       n_child_sum_items +
                       item_list.elements +
                       select_n_reserved +
                       select_n_having_items +
                       select_n_where_fields +
                       order_group_num +
                       hidden_bit_fields +
                       fields_in_window_functions) * 5;
  if (!ref_pointer_array.is_null())
  {
    /*
      We need to take 'n_sum_items' into account when allocating the array,
      and this may actually increase during the optimization phase due to
      MIN/MAX rewrite in Item_in_subselect::single_value_transformer.
      In the usual case we can reuse the array from the prepare phase.
      If we need a bigger array, we must allocate a new one.
     */
    if (ref_pointer_array.size() >= n_elems)
      return false;
   }
  Item **array= static_cast<Item**>(arena->alloc(sizeof(Item*) * n_elems));
  if (array != NULL)
    ref_pointer_array= Ref_ptr_array(array, n_elems);

  return array == NULL;
}