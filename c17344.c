push_new_name_resolution_context(THD *thd,
                                 TABLE_LIST *left_op, TABLE_LIST *right_op)
{
  Name_resolution_context *on_context;
  if (!(on_context= new (thd->mem_root) Name_resolution_context))
    return TRUE;
  on_context->init();
  on_context->first_name_resolution_table=
    left_op->first_leaf_for_name_resolution();
  on_context->last_name_resolution_table=
    right_op->last_leaf_for_name_resolution();
  LEX *lex= thd->lex;
  on_context->select_lex = lex->current_select;
  st_select_lex *curr_select= lex->pop_select();
  st_select_lex *outer_sel= lex->select_stack_head();
  lex->push_select(curr_select);
  on_context->outer_context = outer_sel ? &outer_sel->context : 0;
  return lex->push_context(on_context);
}