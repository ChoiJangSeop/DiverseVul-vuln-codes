int compare_window_funcs_by_window_specs(Item_window_func *win_func1,
                                         Item_window_func *win_func2,
                                         void *arg)
{
  int cmp;
  Window_spec *win_spec1= win_func1->window_spec;
  Window_spec *win_spec2= win_func2->window_spec;
  if (win_spec1 == win_spec2)
    return CMP_EQ;
  cmp= compare_order_lists(win_spec1->partition_list, 
                           win_spec2->partition_list);
  if (cmp == CMP_EQ)
  {
    /* 
      Partition lists contain the same elements. 
      Let's use only one of the lists.
    */
    if (!win_spec1->name() && win_spec2->name())
    {
      win_spec1->save_partition_list= win_spec1->partition_list;
      win_spec1->partition_list= win_spec2->partition_list;
    }
    else
    {
      win_spec2->save_partition_list= win_spec2->partition_list;
      win_spec2->partition_list= win_spec1->partition_list;
    }

    cmp= compare_order_lists(win_spec1->order_list,
                             win_spec2->order_list);

    if (cmp != CMP_EQ)
      return cmp;

    /* 
       Order lists contain the same elements.
       Let's use only one of the lists.
    */
    if (!win_spec1->name() && win_spec2->name())
    {
      win_spec1->save_order_list= win_spec2->order_list;
      win_spec1->order_list= win_spec2->order_list;
    }
    else
    {
      win_spec1->save_order_list= win_spec2->order_list;
      win_spec2->order_list= win_spec1->order_list;
    }

    cmp= compare_window_frames(win_spec1->window_frame,
                               win_spec2->window_frame);

    if (cmp != CMP_EQ)
      return cmp;

    /* Window frames are equal. Let's use only one of them. */
    if (!win_spec1->name() && win_spec2->name())
      win_spec1->window_frame= win_spec2->window_frame;
    else
      win_spec2->window_frame= win_spec1->window_frame;

    return CMP_EQ;
  }
  
  if (cmp == CMP_GT || cmp == CMP_LT)
    return cmp;

  /* one of the partitions lists is the proper beginning of the another */
  cmp= compare_window_spec_joined_lists(win_spec1, win_spec2);

  if (CMP_LT_C <= cmp && cmp <= CMP_GT_C) 
    cmp= win_spec1->partition_list->elements <
      win_spec2->partition_list->elements ? CMP_GT_C : CMP_LT_C;

  return cmp;
}