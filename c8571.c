void
yyerror(YYLTYPE *yylloc, void *scanner, struct yang_parameter *param, ...)
{
  free(*param->value);
  if (yylloc->first_line != -1) {
    if (*param->data_node && (*param->data_node) == (*param->actual_node)) {
      LOGVAL(param->module->ctx, LYE_INSTMT, LY_VLOG_LYS, *param->data_node, yyget_text(scanner));
    } else {
      LOGVAL(param->module->ctx, LYE_INSTMT, LY_VLOG_NONE, NULL, yyget_text(scanner));
    }
  }