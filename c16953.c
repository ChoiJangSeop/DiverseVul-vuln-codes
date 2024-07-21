run_pending_traps ()
{
  register int sig;
  int old_exit_value, x;
  WORD_LIST *save_subst_varlist;
  sh_parser_state_t pstate;
#if defined (ARRAY_VARS)
  ARRAY *ps;
#endif

  if (catch_flag == 0)		/* simple optimization */
    return;

  if (running_trap > 0)
    {
#if defined (DEBUG)
      internal_warning ("run_pending_traps: recursive invocation while running trap for signal %d", running_trap-1);
#endif
#if defined (SIGWINCH)
      if (running_trap == SIGWINCH+1 && pending_traps[SIGWINCH])
	return;			/* no recursive SIGWINCH trap invocations */
#else
      ;
#endif
    }

  catch_flag = trapped_signal_received = 0;

  /* Preserve $? when running trap. */
  trap_saved_exit_value = old_exit_value = last_command_exit_value;
#if defined (ARRAY_VARS)
  ps = save_pipestatus_array ();
#endif

  for (sig = 1; sig < NSIG; sig++)
    {
      /* XXX this could be made into a counter by using
	 while (pending_traps[sig]--) instead of the if statement. */
      if (pending_traps[sig])
	{
	  if (running_trap == sig+1)
	    /*continue*/;

	  running_trap = sig + 1;

	  if (sig == SIGINT)
	    {
	      pending_traps[sig] = 0;	/* XXX */
	      run_interrupt_trap (0);
	      CLRINTERRUPT;
	    }
#if defined (JOB_CONTROL) && defined (SIGCHLD)
	  else if (sig == SIGCHLD &&
		   trap_list[SIGCHLD] != (char *)IMPOSSIBLE_TRAP_HANDLER &&
		   (sigmodes[SIGCHLD] & SIG_INPROGRESS) == 0)
	    {
	      sigmodes[SIGCHLD] |= SIG_INPROGRESS;
	      x = pending_traps[sig];
	      pending_traps[sig] = 0;
	      run_sigchld_trap (x);	/* use as counter */
	      running_trap = 0;
	      sigmodes[SIGCHLD] &= ~SIG_INPROGRESS;
	      /* continue here rather than reset pending_traps[SIGCHLD] below in
		 case there are recursive calls to run_pending_traps and children
		 have been reaped while run_sigchld_trap was running. */
	      continue;
	    }
	  else if (sig == SIGCHLD &&
		   trap_list[SIGCHLD] == (char *)IMPOSSIBLE_TRAP_HANDLER &&
		   (sigmodes[SIGCHLD] & SIG_INPROGRESS) != 0)
	    {
	      /* This can happen when run_pending_traps is called while
		 running a SIGCHLD trap handler. */
	      running_trap = 0;
	      /* want to leave pending_traps[SIGCHLD] alone here */
	      continue;					/* XXX */
	    }
	  else if (sig == SIGCHLD && (sigmodes[SIGCHLD] & SIG_INPROGRESS))
	    {
	      /* whoops -- print warning? */
	      running_trap = 0;		/* XXX */
	      /* want to leave pending_traps[SIGCHLD] alone here */
	      continue;
	    }
#endif
	  else if (trap_list[sig] == (char *)DEFAULT_SIG ||
		   trap_list[sig] == (char *)IGNORE_SIG ||
		   trap_list[sig] == (char *)IMPOSSIBLE_TRAP_HANDLER)
	    {
	      /* This is possible due to a race condition.  Say a bash
		 process has SIGTERM trapped.  A subshell is spawned
		 using { list; } & and the parent does something and kills
		 the subshell with SIGTERM.  It's possible for the subshell
		 to set pending_traps[SIGTERM] to 1 before the code in
		 execute_cmd.c eventually calls restore_original_signals
		 to reset the SIGTERM signal handler in the subshell.  The
		 next time run_pending_traps is called, pending_traps[SIGTERM]
		 will be 1, but the trap handler in trap_list[SIGTERM] will
		 be invalid (probably DEFAULT_SIG, but it could be IGNORE_SIG).
		 Unless we catch this, the subshell will dump core when
		 trap_list[SIGTERM] == DEFAULT_SIG, because DEFAULT_SIG is
		 usually 0x0. */
	      internal_warning (_("run_pending_traps: bad value in trap_list[%d]: %p"),
				sig, trap_list[sig]);
	      if (trap_list[sig] == (char *)DEFAULT_SIG)
		{
		  internal_warning (_("run_pending_traps: signal handler is SIG_DFL, resending %d (%s) to myself"), sig, signal_name (sig));
		  kill (getpid (), sig);
		}
	    }
	  else
	    {
	      /* XXX - should we use save_parser_state/restore_parser_state? */
	      save_parser_state (&pstate);
	      save_subst_varlist = subst_assign_varlist;
	      subst_assign_varlist = 0;

#if defined (JOB_CONTROL)
	      save_pipeline (1);	/* XXX only provides one save level */
#endif
	      /* XXX - set pending_traps[sig] = 0 here? */
	      pending_traps[sig] = 0;
	      evalstring (savestring (trap_list[sig]), "trap", SEVAL_NONINT|SEVAL_NOHIST|SEVAL_RESETLINE);
#if defined (JOB_CONTROL)
	      restore_pipeline (1);
#endif

	      subst_assign_varlist = save_subst_varlist;
	      restore_parser_state (&pstate);
	    }

	  pending_traps[sig] = 0;	/* XXX - move before evalstring? */
	  running_trap = 0;
	}
    }

#if defined (ARRAY_VARS)
  restore_pipestatus_array (ps);
#endif
  last_command_exit_value = old_exit_value;
}