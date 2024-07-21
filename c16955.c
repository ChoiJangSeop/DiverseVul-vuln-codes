initialize_subshell ()
{
#if defined (ALIAS)
  /* Forget about any aliases that we knew of.  We are in a subshell. */
  delete_all_aliases ();
#endif /* ALIAS */

#if defined (HISTORY)
  /* Forget about the history lines we have read.  This is a non-interactive
     subshell. */
  history_lines_this_session = 0;
#endif

#if defined (JOB_CONTROL)
  /* Forget about the way job control was working. We are in a subshell. */
  without_job_control ();
  set_sigchld_handler ();
  init_job_stats ();
#endif /* JOB_CONTROL */

  /* Reset the values of the shell flags and options. */
  reset_shell_flags ();
  reset_shell_options ();
  reset_shopt_options ();

  /* Zero out builtin_env, since this could be a shell script run from a
     sourced file with a temporary environment supplied to the `source/.'
     builtin.  Such variables are not supposed to be exported (empirical
     testing with sh and ksh).  Just throw it away; don't worry about a
     memory leak. */
  if (vc_isbltnenv (shell_variables))
    shell_variables = shell_variables->down;

  clear_unwind_protect_list (0);
  /* XXX -- are there other things we should be resetting here? */
  parse_and_execute_level = 0;		/* nothing left to restore it */

  /* We're no longer inside a shell function. */
  variable_context = return_catch_flag = funcnest = evalnest = sourcenest = 0;

  executing_list = 0;		/* XXX */

  /* If we're not interactive, close the file descriptor from which we're
     reading the current shell script. */
  if (interactive_shell == 0)
    unset_bash_input (0);
}