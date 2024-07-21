void LEX::relink_hack(st_select_lex *select_lex)
{
  if (!select_stack_top) // Statements of the second type
  {
    if (!select_lex->get_master()->get_master())
      ((st_select_lex *) select_lex->get_master())->
        set_master(&builtin_select);
    if (!builtin_select.get_slave())
      builtin_select.set_slave(select_lex->get_master());
  }
}