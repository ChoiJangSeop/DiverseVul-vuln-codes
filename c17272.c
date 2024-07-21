int TABLE::fix_vcol_exprs(THD *thd)
{
  for (Field **vf= vfield; vf && *vf; vf++)
    if (fix_session_vcol_expr(thd, (*vf)->vcol_info))
      return 1;

  for (Field **df= default_field; df && *df; df++)
    if ((*df)->default_value &&
        fix_session_vcol_expr(thd, (*df)->default_value))
      return 1;

  for (Virtual_column_info **cc= check_constraints; cc && *cc; cc++)
    if (fix_session_vcol_expr(thd, (*cc)))
      return 1;

  return 0;
}