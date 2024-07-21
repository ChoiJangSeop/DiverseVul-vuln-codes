_run_trap_internal (sig, tag)
     int sig;
     char *tag;
{
  char *trap_command, *old_trap;
  int trap_exit_value, *token_state;
  volatile int save_return_catch_flag, function_code, top_level_code, old_int;
  int flags;
  procenv_t save_return_catch;
  WORD_LIST *save_subst_varlist;
  sh_parser_state_t pstate;
#if defined (ARRAY_VARS)
  ARRAY *ps;
#endif

  trap_exit_value = function_code = 0;
  trap_saved_exit_value = last_command_exit_value;
  /* Run the trap only if SIG is trapped and not ignored, and we are not
     currently executing in the trap handler. */
  if ((sigmodes[sig] & SIG_TRAPPED) && ((sigmodes[sig] & SIG_IGNORED) == 0) &&
      (trap_list[sig] != (char *)IMPOSSIBLE_TRAP_HANDLER) &&
#if 0
      /* Uncomment this to allow some special signals to recursively execute
	 trap handlers. */
      (RECURSIVE_SIG (sig) || (sigmodes[sig] & SIG_INPROGRESS) == 0))
#else
      ((sigmodes[sig] & SIG_INPROGRESS) == 0))
#endif
    {
      old_trap = trap_list[sig];
      sigmodes[sig] |= SIG_INPROGRESS;
      sigmodes[sig] &= ~SIG_CHANGED;		/* just to be sure */
      trap_command =  savestring (old_trap);

      running_trap = sig + 1;

      old_int = interrupt_state;	/* temporarily suppress pending interrupts */
      CLRINTERRUPT;

#if defined (ARRAY_VARS)
      ps = save_pipestatus_array ();
#endif

      save_parser_state (&pstate);
      save_subst_varlist = subst_assign_varlist;
      subst_assign_varlist = 0;

#if defined (JOB_CONTROL)
      if (sig != DEBUG_TRAP)	/* run_debug_trap does this */
	save_pipeline (1);	/* XXX only provides one save level */
#endif

      /* If we're in a function, make sure return longjmps come here, too. */
      save_return_catch_flag = return_catch_flag;
      if (return_catch_flag)
	{
	  COPY_PROCENV (return_catch, save_return_catch);
	  function_code = setjmp_nosigs (return_catch);
	}

      flags = SEVAL_NONINT|SEVAL_NOHIST;
      if (sig != DEBUG_TRAP && sig != RETURN_TRAP && sig != ERROR_TRAP)
	flags |= SEVAL_RESETLINE;
      if (function_code == 0)
        {
	  parse_and_execute (trap_command, tag, flags);
	  trap_exit_value = last_command_exit_value;
        }
      else
        trap_exit_value = return_catch_value;

#if defined (JOB_CONTROL)
      if (sig != DEBUG_TRAP)	/* run_debug_trap does this */
	restore_pipeline (1);
#endif

      subst_assign_varlist = save_subst_varlist;
      restore_parser_state (&pstate);

#if defined (ARRAY_VARS)
      restore_pipestatus_array (ps);
#endif

      sigmodes[sig] &= ~SIG_INPROGRESS;
      running_trap = 0;
      interrupt_state = old_int;

      if (sigmodes[sig] & SIG_CHANGED)
	{
#if 0
	  /* Special traps like EXIT, DEBUG, RETURN are handled explicitly in
	     the places where they can be changed using unwind-protects.  For
	     example, look at execute_cmd.c:execute_function(). */
	  if (SPECIAL_TRAP (sig) == 0)
#endif
	    free (old_trap);
	  sigmodes[sig] &= ~SIG_CHANGED;
	}

      if (save_return_catch_flag)
	{
	  return_catch_flag = save_return_catch_flag;
	  return_catch_value = trap_exit_value;
	  COPY_PROCENV (save_return_catch, return_catch);
	  if (function_code)
	    {
#if 0
	      from_return_trap = sig == RETURN_TRAP;
#endif
	      sh_longjmp (return_catch, 1);
	    }
	}
    }

  return trap_exit_value;
}