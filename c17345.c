void LEX::start(THD *thd_arg)
{
  DBUG_ENTER("LEX::start");
  DBUG_PRINT("info", ("This: %p thd_arg->lex: %p", this, thd_arg->lex));

  thd= unit.thd= thd_arg;
  stmt_lex= this; // default, should be rewritten for VIEWs And CTEs

  DBUG_ASSERT(!explain);

  builtin_select.lex_start(this);
  lex_options= 0;
  context_stack.empty();
  //empty select_stack
  select_stack_top= 0;
  unit.init_query();
  current_select_number= 0;
  curr_with_clause= 0;
  with_clauses_list= 0;
  with_clauses_list_last_next= &with_clauses_list;
  clone_spec_offset= 0;
  create_view= NULL;
  field_list.empty();
  value_list.empty();
  update_list.empty();
  set_var_list.empty();
  param_list.empty();
  view_list.empty();
  with_column_list.empty();
  with_persistent_for_clause= FALSE;
  column_list= NULL;
  index_list= NULL;
  prepared_stmt.lex_start();
  auxiliary_table_list.empty();
  unit.next= unit.master= unit.link_next= unit.return_to= 0;
  unit.prev= unit.link_prev= 0;
  unit.slave= current_select= all_selects_list= &builtin_select;
  sql_cache= LEX::SQL_CACHE_UNSPECIFIED;
  describe= 0;
  analyze_stmt= 0;
  explain_json= false;
  context_analysis_only= 0;
  derived_tables= 0;
  with_cte_resolution= false;
  only_cte_resolution= false;
  safe_to_cache_query= 1;
  parsing_options.reset();
  empty_field_list_on_rset= 0;
  part_info= 0;
  m_sql_cmd= NULL;
  duplicates= DUP_ERROR;
  ignore= 0;
  spname= NULL;
  spcont= NULL;
  proc_list.first= 0;
  escape_used= FALSE;
  default_used= FALSE;
  query_tables= 0;
  reset_query_tables_list(FALSE);
  clause_that_disallows_subselect= NULL;
  selects_allow_into= FALSE;
  selects_allow_procedure= FALSE;
  use_only_table_context= FALSE;
  parse_vcol_expr= FALSE;
  check_exists= FALSE;
  create_info.lex_start();
  verbose= 0;

  name= null_clex_str;
  event_parse_data= NULL;
  profile_options= PROFILE_NONE;
  nest_level= 0;
  builtin_select.nest_level_base= &unit;
  allow_sum_func.clear_all();
  in_sum_func= NULL;

  used_tables= 0;
  table_type= TABLE_TYPE_UNKNOWN;
  reset_slave_info.all= false;
  limit_rows_examined= 0;
  limit_rows_examined_cnt= ULONGLONG_MAX;
  var_list.empty();
  stmt_var_list.empty();
  proc_list.elements=0;

  save_group_list.empty();
  save_order_list.empty();
  win_ref= NULL;
  win_frame= NULL;
  frame_top_bound= NULL;
  frame_bottom_bound= NULL;
  win_spec= NULL;

  vers_conditions.empty();
  period_conditions.empty();

  is_lex_started= TRUE;

  next_is_main= FALSE;
  next_is_down= FALSE;

  wild= 0;
  exchange= 0;

  DBUG_VOID_RETURN;
}