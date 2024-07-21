execute_arith_command (arith_command)
     ARITH_COM *arith_command;
{
  int expok, save_line_number, retval;
  intmax_t expresult;
  WORD_LIST *new;
  char *exp;

  expresult = 0;

  save_line_number = line_number;
  this_command_name = "((";	/* )) */
  line_number = arith_command->line;
  /* If we're in a function, update the line number information. */
  if (variable_context && interactive_shell)
    line_number -= function_line_number;

  command_string_index = 0;
  print_arith_command (arith_command->exp);

  if (signal_in_progress (DEBUG_TRAP) == 0)
    {
      FREE (the_printed_command_except_trap);
      the_printed_command_except_trap = savestring (the_printed_command);
    }

  /* Run the debug trap before each arithmetic command, but do it after we
     update the line number information and before we expand the various
     words in the expression. */
  retval = run_debug_trap ();
#if defined (DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && retval != EXECUTION_SUCCESS)
    {
      line_number = save_line_number;
      return (EXECUTION_SUCCESS);
    }
#endif

  new = expand_words_no_vars (arith_command->exp);

  /* If we're tracing, make a new word list with `((' at the front and `))'
     at the back and print it. */
  if (echo_command_at_execute)
    xtrace_print_arith_cmd (new);

  if (new)
    {
      exp = new->next ? string_list (new) : new->word->word;
      expresult = evalexp (exp, &expok);
      line_number = save_line_number;
      if (exp != new->word->word)
	free (exp);
      dispose_words (new);
    }
  else
    {
      expresult = 0;
      expok = 1;
    }

  if (expok == 0)
    return (EXECUTION_FAILURE);

  return (expresult == 0 ? EXECUTION_FAILURE : EXECUTION_SUCCESS);
}