bool TABLE::vcol_fix_exprs(THD *thd)
{
  if (pos_in_table_list->placeholder() || !s->vcols_need_refixing ||
      pos_in_table_list->lock_type < TL_WRITE_ALLOW_WRITE)
    return false;

  DBUG_ASSERT(pos_in_table_list != thd->lex->first_not_own_table());

  bool result= true;
  Security_context *save_security_ctx= thd->security_ctx;
  Query_arena *stmt_backup= thd->stmt_arena;
  if (thd->stmt_arena->is_conventional())
    thd->stmt_arena= expr_arena;

  if (pos_in_table_list->security_ctx)
    thd->security_ctx= pos_in_table_list->security_ctx;


  for (Field **vf= vfield; vf && *vf; vf++)
    if ((*vf)->vcol_info->fix_session_expr(thd))
      goto end;

  for (Field **df= default_field; df && *df; df++)
    if ((*df)->default_value &&
        (*df)->default_value->fix_session_expr(thd))
      goto end;

  for (Virtual_column_info **cc= check_constraints; cc && *cc; cc++)
    if ((*cc)->fix_session_expr(thd))
      goto end;

  result= false;

end:
  thd->security_ctx= save_security_ctx;
  thd->stmt_arena= stmt_backup;

  return result;
}