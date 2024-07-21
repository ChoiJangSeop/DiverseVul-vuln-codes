int compare_window_spec_joined_lists(Window_spec *win_spec1,
                                     Window_spec *win_spec2)
{
  win_spec1->join_partition_and_order_lists();
  win_spec2->join_partition_and_order_lists();
  int cmp= compare_order_lists(win_spec1->partition_list, 
                               win_spec2->partition_list);
  win_spec1->disjoin_partition_and_order_lists();
  win_spec2->disjoin_partition_and_order_lists();
  return cmp;
}