execute_arith_for_command (arith_for_command)
     ARITH_FOR_COM *arith_for_command;
{
  intmax_t expresult;
  int expok, body_status, arith_lineno, save_lineno;

  body_status = EXECUTION_SUCCESS;
  loop_level++;
  save_lineno = line_number;

  if (arith_for_command->flags & CMD_IGNORE_RETURN)
    arith_for_command->action->flags |= CMD_IGNORE_RETURN;

  this_command_name = "((";	/* )) for expression error messages */

  /* save the starting line number of the command so we can reset
     line_number before executing each expression -- for $LINENO
     and the DEBUG trap. */
  line_number = arith_lineno = arith_for_command->line;
  if (variable_context && interactive_shell)
    line_number -= function_line_number;

  /* Evaluate the initialization expression. */
  expresult = eval_arith_for_expr (arith_for_command->init, &expok);
  if (expok == 0)
    {
      line_number = save_lineno;
      return (EXECUTION_FAILURE);
    }

  while (1)
    {
      /* Evaluate the test expression. */
      line_number = arith_lineno;
      expresult = eval_arith_for_expr (arith_for_command->test, &expok);
      line_number = save_lineno;

      if (expok == 0)
	{
	  body_status = EXECUTION_FAILURE;
	  break;
	}
      REAP ();
      if (expresult == 0)
	break;

      /* Execute the body of the arithmetic for command. */
      QUIT;
      body_status = execute_command (arith_for_command->action);
      QUIT;

      /* Handle any `break' or `continue' commands executed by the body. */
      if (breaking)
	{
	  breaking--;
	  break;
	}

      if (continuing)
	{
	  continuing--;
	  if (continuing)
	    break;
	}

      /* Evaluate the step expression. */
      line_number = arith_lineno;
      expresult = eval_arith_for_expr (arith_for_command->step, &expok);
      line_number = save_lineno;

      if (expok == 0)
	{
	  body_status = EXECUTION_FAILURE;
	  break;
	}
    }

  loop_level--;
  line_number = save_lineno;

  return (body_status);
}