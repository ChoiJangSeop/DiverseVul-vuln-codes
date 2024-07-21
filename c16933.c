run_sigchld_trap (nchild)
     int nchild;
{
  char *trap_command;
  int i;

  /* Turn off the trap list during the call to parse_and_execute ()
     to avoid potentially infinite recursive calls.  Preserve the
     values of last_command_exit_value, last_made_pid, and the_pipeline
     around the execution of the trap commands. */
  trap_command = savestring (trap_list[SIGCHLD]);

  begin_unwind_frame ("SIGCHLD trap");
  unwind_protect_int (last_command_exit_value);
  unwind_protect_int (last_command_exit_signal);
  unwind_protect_var (last_made_pid);
  unwind_protect_int (interrupt_immediately);
  unwind_protect_int (jobs_list_frozen);
  unwind_protect_pointer (the_pipeline);
  unwind_protect_pointer (subst_assign_varlist);
  unwind_protect_pointer (this_shell_builtin);

  /* We have to add the commands this way because they will be run
     in reverse order of adding.  We don't want maybe_set_sigchld_trap ()
     to reference freed memory. */
  add_unwind_protect (xfree, trap_command);
  add_unwind_protect (maybe_set_sigchld_trap, trap_command);

  subst_assign_varlist = (WORD_LIST *)NULL;
  the_pipeline = (PROCESS *)NULL;

  running_trap = SIGCHLD + 1;

  set_impossible_sigchld_trap ();
  jobs_list_frozen = 1;
  for (i = 0; i < nchild; i++)
    {
#if 0
      interrupt_immediately = 1;
#endif
      parse_and_execute (savestring (trap_command), "trap", SEVAL_NOHIST|SEVAL_RESETLINE);
    }

  run_unwind_frame ("SIGCHLD trap");
  running_trap = 0;
}