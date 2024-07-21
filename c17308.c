bool st_select_lex::add_window_spec(THD *thd, 
                                    LEX_STRING *win_ref,
                                    SQL_I_List<ORDER> win_partition_list,
                                    SQL_I_List<ORDER> win_order_list,
                                    Window_frame *win_frame)
{
  SQL_I_List<ORDER> *win_part_list_ptr=
    new (thd->mem_root) SQL_I_List<ORDER> (win_partition_list);
  SQL_I_List<ORDER> *win_order_list_ptr=
    new (thd->mem_root) SQL_I_List<ORDER> (win_order_list);
  if (!(win_part_list_ptr && win_order_list_ptr))
    return true;
  Window_spec *win_spec= new (thd->mem_root) Window_spec(win_ref,
                                                         win_part_list_ptr,
                                                         win_order_list_ptr,
                                                         win_frame);
  group_list= thd->lex->save_group_list;
  order_list= thd->lex->save_order_list;
  if (parsing_place != SELECT_LIST)
  {
    fields_in_window_functions+= win_part_list_ptr->elements +
                                 win_order_list_ptr->elements;
  }
  thd->lex->win_spec= win_spec;
  return (win_spec == NULL || window_specs.push_back(win_spec));
}