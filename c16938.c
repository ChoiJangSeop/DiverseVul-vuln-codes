execute_simple_command (simple_command, pipe_in, pipe_out, async, fds_to_close)
     SIMPLE_COM *simple_command;
     int pipe_in, pipe_out, async;
     struct fd_bitmap *fds_to_close;
{
  WORD_LIST *words, *lastword;
  char *command_line, *lastarg, *temp;
  int first_word_quoted, result, builtin_is_special, already_forked, dofork;
  pid_t old_last_async_pid;
  sh_builtin_func_t *builtin;
  SHELL_VAR *func;
  volatile int old_builtin, old_command_builtin;

  result = EXECUTION_SUCCESS;
  special_builtin_failed = builtin_is_special = 0;
  command_line = (char *)0;

  QUIT;

  /* If we're in a function, update the line number information. */
  if (variable_context && interactive_shell && sourcelevel == 0)
    line_number -= function_line_number;

  /* Remember what this command line looks like at invocation. */
  command_string_index = 0;
  print_simple_command (simple_command);

#if 0
  if (signal_in_progress (DEBUG_TRAP) == 0 && (this_command_name == 0 || (STREQ (this_command_name, "trap") == 0)))
#else
  if (signal_in_progress (DEBUG_TRAP) == 0 && running_trap == 0)
#endif
    {
      FREE (the_printed_command_except_trap);
      the_printed_command_except_trap = the_printed_command ? savestring (the_printed_command) : (char *)0;
    }

  /* Run the debug trap before each simple command, but do it after we
     update the line number information. */
  result = run_debug_trap ();
#if defined (DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && result != EXECUTION_SUCCESS)
    return (EXECUTION_SUCCESS);
#endif

  first_word_quoted =
    simple_command->words ? (simple_command->words->word->flags & W_QUOTED) : 0;

  last_command_subst_pid = NO_PID;
  old_last_async_pid = last_asynchronous_pid;

  already_forked = dofork = 0;

  /* If we're in a pipeline or run in the background, set DOFORK so we
     make the child early, before word expansion.  This keeps assignment
     statements from affecting the parent shell's environment when they
     should not. */
  dofork = pipe_in != NO_PIPE || pipe_out != NO_PIPE || async;

  /* Something like `%2 &' should restart job 2 in the background, not cause
     the shell to fork here. */
  if (dofork && pipe_in == NO_PIPE && pipe_out == NO_PIPE &&
	simple_command->words && simple_command->words->word &&
	simple_command->words->word->word &&
	(simple_command->words->word->word[0] == '%'))
    dofork = 0;

  if (dofork)
    {
      /* Do this now, because execute_disk_command will do it anyway in the
	 vast majority of cases. */
      maybe_make_export_env ();

      /* Don't let a DEBUG trap overwrite the command string to be saved with
	 the process/job associated with this child. */
      if (make_child (savestring (the_printed_command_except_trap), async) == 0)
	{
	  already_forked = 1;
	  simple_command->flags |= CMD_NO_FORK;

	  subshell_environment = SUBSHELL_FORK;		/* XXX */
	  if (pipe_in != NO_PIPE || pipe_out != NO_PIPE)
	    subshell_environment |= SUBSHELL_PIPE;
	  if (async)
	    subshell_environment |= SUBSHELL_ASYNC;

	  /* We need to do this before piping to handle some really
	     pathological cases where one of the pipe file descriptors
	     is < 2. */
	  if (fds_to_close)
	    close_fd_bitmap (fds_to_close);

	  /* If we fork because of an input pipe, note input pipe for later to
	     inhibit async commands from redirecting stdin from /dev/null */
	  stdin_redir |= pipe_in != NO_PIPE;

	  do_piping (pipe_in, pipe_out);
	  pipe_in = pipe_out = NO_PIPE;
#if defined (COPROCESS_SUPPORT)
	  coproc_closeall ();
#endif

	  last_asynchronous_pid = old_last_async_pid;

	  CHECK_SIGTERM;

	  if (async)
	    subshell_level++;		/* not for pipes yet */
	}
      else
	{
	  /* Don't let simple commands that aren't the last command in a
	     pipeline change $? for the rest of the pipeline (or at all). */
	  if (pipe_out != NO_PIPE)
	    result = last_command_exit_value;
	  close_pipes (pipe_in, pipe_out);
#if defined (PROCESS_SUBSTITUTION) && defined (HAVE_DEV_FD)
	  /* Close /dev/fd file descriptors in the parent after forking the
	     last child in a (possibly one-element) pipeline.  Defer this
	     until any running shell function completes. */
	  if (pipe_out == NO_PIPE && variable_context == 0)	/* XXX */
	    unlink_fifo_list ();		/* XXX */
#endif
	  command_line = (char *)NULL;      /* don't free this. */
	  bind_lastarg ((char *)NULL);
	  return (result);
	}
    }

  QUIT;		/* XXX */

  /* If we are re-running this as the result of executing the `command'
     builtin, do not expand the command words a second time. */
  if ((simple_command->flags & CMD_INHIBIT_EXPANSION) == 0)
    {
      current_fds_to_close = fds_to_close;
      fix_assignment_words (simple_command->words);
      /* Pass the ignore return flag down to command substitutions */
      if (simple_command->flags & CMD_IGNORE_RETURN)	/* XXX */
	comsub_ignore_return++;
      words = expand_words (simple_command->words);
      if (simple_command->flags & CMD_IGNORE_RETURN)
	comsub_ignore_return--;
      current_fds_to_close = (struct fd_bitmap *)NULL;
    }
  else
    words = copy_word_list (simple_command->words);

  /* It is possible for WORDS not to have anything left in it.
     Perhaps all the words consisted of `$foo', and there was
     no variable `$foo'. */
  if (words == 0)
    {
      this_command_name = 0;
      result = execute_null_command (simple_command->redirects,
				     pipe_in, pipe_out,
				     already_forked ? 0 : async);
      if (already_forked)
	sh_exit (result);
      else
	{
	  bind_lastarg ((char *)NULL);
	  set_pipestatus_from_exit (result);
	  return (result);
	}
    }

  lastarg = (char *)NULL;

  begin_unwind_frame ("simple-command");

  if (echo_command_at_execute)
    xtrace_print_word_list (words, 1);

  builtin = (sh_builtin_func_t *)NULL;
  func = (SHELL_VAR *)NULL;
  if ((simple_command->flags & CMD_NO_FUNCTIONS) == 0)
    {
      /* Posix.2 says special builtins are found before functions.  We
	 don't set builtin_is_special anywhere other than here, because
	 this path is followed only when the `command' builtin is *not*
	 being used, and we don't want to exit the shell if a special
	 builtin executed with `command builtin' fails.  `command' is not
	 a special builtin. */
      if (posixly_correct)
	{
	  builtin = find_special_builtin (words->word->word);
	  if (builtin)
	    builtin_is_special = 1;
	}
      if (builtin == 0)
	func = find_function (words->word->word);
    }

  /* In POSIX mode, assignment errors in the temporary environment cause a
     non-interactive shell to exit. */
  if (posixly_correct && builtin_is_special && interactive_shell == 0 && tempenv_assign_error)
    {
      last_command_exit_value = EXECUTION_FAILURE;
      jump_to_top_level (ERREXIT);
    }
  tempenv_assign_error = 0;	/* don't care about this any more */

  add_unwind_protect (dispose_words, words);
  QUIT;

  /* Bind the last word in this command to "$_" after execution. */
  for (lastword = words; lastword->next; lastword = lastword->next)
    ;
  lastarg = lastword->word->word;

#if defined (JOB_CONTROL)
  /* Is this command a job control related thing? */
  if (words->word->word[0] == '%' && already_forked == 0)
    {
      this_command_name = async ? "bg" : "fg";
      last_shell_builtin = this_shell_builtin;
      this_shell_builtin = builtin_address (this_command_name);
      result = (*this_shell_builtin) (words);
      goto return_result;
    }

  /* One other possibililty.  The user may want to resume an existing job.
     If they do, find out whether this word is a candidate for a running
     job. */
  if (job_control && already_forked == 0 && async == 0 &&
	!first_word_quoted &&
	!words->next &&
	words->word->word[0] &&
	!simple_command->redirects &&
	pipe_in == NO_PIPE &&
	pipe_out == NO_PIPE &&
	(temp = get_string_value ("auto_resume")))
    {
      int job, jflags, started_status;

      jflags = JM_STOPPED|JM_FIRSTMATCH;
      if (STREQ (temp, "exact"))
	jflags |= JM_EXACT;
      else if (STREQ (temp, "substring"))
	jflags |= JM_SUBSTRING;
      else
	jflags |= JM_PREFIX;
      job = get_job_by_name (words->word->word, jflags);
      if (job != NO_JOB)
	{
	  run_unwind_frame ("simple-command");
	  this_command_name = "fg";
	  last_shell_builtin = this_shell_builtin;
	  this_shell_builtin = builtin_address ("fg");

	  started_status = start_job (job, 1);
	  return ((started_status < 0) ? EXECUTION_FAILURE : started_status);
	}
    }
#endif /* JOB_CONTROL */

run_builtin:
  /* Remember the name of this command globally. */
  this_command_name = words->word->word;

  QUIT;

  /* This command could be a shell builtin or a user-defined function.
     We have already found special builtins by this time, so we do not
     set builtin_is_special.  If this is a function or builtin, and we
     have pipes, then fork a subshell in here.  Otherwise, just execute
     the command directly. */
  if (func == 0 && builtin == 0)
    builtin = find_shell_builtin (this_command_name);

  last_shell_builtin = this_shell_builtin;
  this_shell_builtin = builtin;

  if (builtin || func)
    {
      if (builtin)
        {
	  old_builtin = executing_builtin;
	  old_command_builtin = executing_command_builtin;
	  unwind_protect_int (executing_builtin);	/* modified in execute_builtin */
	  unwind_protect_int (executing_command_builtin);	/* ditto */
        }
      if (already_forked)
	{
	  /* reset_terminating_signals (); */	/* XXX */
	  /* Reset the signal handlers in the child, but don't free the
	     trap strings.  Set a flag noting that we have to free the
	     trap strings if we run trap to change a signal disposition. */
	  reset_signal_handlers ();
	  subshell_environment |= SUBSHELL_RESETTRAP;

	  if (async)
	    {
	      if ((simple_command->flags & CMD_STDIN_REDIR) &&
		    pipe_in == NO_PIPE &&
		    (stdin_redirects (simple_command->redirects) == 0))
		async_redirect_stdin ();
	      setup_async_signals ();
	    }

	  if (async == 0)
	    subshell_level++;
	  execute_subshell_builtin_or_function
	    (words, simple_command->redirects, builtin, func,
	     pipe_in, pipe_out, async, fds_to_close,
	     simple_command->flags);
	  subshell_level--;
	}
      else
	{
	  result = execute_builtin_or_function
	    (words, builtin, func, simple_command->redirects, fds_to_close,
	     simple_command->flags);
	  if (builtin)
	    {
	      if (result > EX_SHERRBASE)
		{
		  switch (result)
		    {
		    case EX_REDIRFAIL:
		    case EX_BADASSIGN:
		    case EX_EXPFAIL:
		      /* These errors cause non-interactive posix mode shells to exit */
		      if (posixly_correct && builtin_is_special && interactive_shell == 0)
			{
			  last_command_exit_value = EXECUTION_FAILURE;
			  jump_to_top_level (ERREXIT);
			}
		    }
		  result = builtin_status (result);
		  if (builtin_is_special)
		    special_builtin_failed = 1;
		}
	      /* In POSIX mode, if there are assignment statements preceding
		 a special builtin, they persist after the builtin
		 completes. */
	      if (posixly_correct && builtin_is_special && temporary_env)
		merge_temporary_env ();
	    }
	  else		/* function */
	    {
	      if (result == EX_USAGE)
		result = EX_BADUSAGE;
	      else if (result > EX_SHERRBASE)
		result = EXECUTION_FAILURE;
	    }

	  set_pipestatus_from_exit (result);

	  goto return_result;
	}
    }

  if (autocd && interactive && words->word && is_dirname (words->word->word))
    {
      words = make_word_list (make_word ("--"), words);
      words = make_word_list (make_word ("cd"), words);
      xtrace_print_word_list (words, 0);
      func = find_function ("cd");
      goto run_builtin;
    }

  if (command_line == 0)
    command_line = savestring (the_printed_command_except_trap ? the_printed_command_except_trap : "");

#if defined (PROCESS_SUBSTITUTION)
  /* The old code did not test already_forked and only did this if
     subshell_environment&SUBSHELL_COMSUB != 0 (comsubs and procsubs). Other
     uses of the no-fork optimization left FIFOs in $TMPDIR */
  if (already_forked == 0 && (simple_command->flags & CMD_NO_FORK) && fifos_pending() > 0)
    simple_command->flags &= ~CMD_NO_FORK;
#endif
  result = execute_disk_command (words, simple_command->redirects, command_line,
			pipe_in, pipe_out, async, fds_to_close,
			simple_command->flags);

 return_result:
  bind_lastarg (lastarg);
  FREE (command_line);
  dispose_words (words);
  if (builtin)
    {
      executing_builtin = old_builtin;
      executing_command_builtin = old_command_builtin;
    }
  discard_unwind_frame ("simple-command");
  this_command_name = (char *)NULL;	/* points to freed memory now */
  return (result);
}