execute_while_or_until (while_command, type)
     WHILE_COM *while_command;
     int type;
{
  int return_value, body_status;

  body_status = EXECUTION_SUCCESS;
  loop_level++;

  while_command->test->flags |= CMD_IGNORE_RETURN;
  if (while_command->flags & CMD_IGNORE_RETURN)
    while_command->action->flags |= CMD_IGNORE_RETURN;

  while (1)
    {
      return_value = execute_command (while_command->test);
      REAP ();

      /* Need to handle `break' in the test when we would break out of the
         loop.  The job control code will set `breaking' to loop_level
         when a job in a loop is stopped with SIGTSTP.  If the stopped job
         is in the loop test, `breaking' will not be reset unless we do
         this, and the shell will cease to execute commands. */
      if (type == CMD_WHILE && return_value != EXECUTION_SUCCESS)
	{
	  if (breaking)
	    breaking--;
	  break;
	}
      if (type == CMD_UNTIL && return_value == EXECUTION_SUCCESS)
	{
	  if (breaking)
	    breaking--;
	  break;
	}

      QUIT;
      body_status = execute_command (while_command->action);
      QUIT;

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
    }
  loop_level--;

  return (body_status);
}