void order_window_funcs_by_window_specs(List<Item_window_func> *win_func_list)
{
  if (win_func_list->elements == 0)
    return;

  bubble_sort<Item_window_func>(win_func_list,
                                compare_window_funcs_by_window_specs,
                                NULL);

  List_iterator_fast<Item_window_func> it(*win_func_list);
  Item_window_func *prev= it++;
  prev->marker= SORTORDER_CHANGE_FLAG |
                PARTITION_CHANGE_FLAG |
                FRAME_CHANGE_FLAG;
  Item_window_func *curr;
  while ((curr= it++))
  {
    Window_spec *win_spec_prev= prev->window_spec;
    Window_spec *win_spec_curr= curr->window_spec;
    curr->marker= 0;
    if (!(win_spec_prev->partition_list == win_spec_curr->partition_list &&
          win_spec_prev->order_list == win_spec_curr->order_list))
    {
      int cmp;
      if (win_spec_prev->partition_list == win_spec_curr->partition_list)
        cmp= compare_order_lists(win_spec_prev->order_list,
                                 win_spec_curr->order_list);
      else
        cmp= compare_window_spec_joined_lists(win_spec_prev, win_spec_curr);
      if (!(CMP_LT_C <= cmp && cmp <= CMP_GT_C))
      {
        curr->marker= SORTORDER_CHANGE_FLAG |
                      PARTITION_CHANGE_FLAG |
                      FRAME_CHANGE_FLAG;
      }
      else if (win_spec_prev->partition_list != win_spec_curr->partition_list)
      {
        curr->marker|= PARTITION_CHANGE_FLAG | FRAME_CHANGE_FLAG;
      }
    }
    else if (win_spec_prev->window_frame != win_spec_curr->window_frame)
      curr->marker|= FRAME_CHANGE_FLAG;

    prev= curr;
  }
}