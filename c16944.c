wait_for_single_pid (pid)
     pid_t pid;
{
  pid_t got_pid;
  WAIT status;
  int pstatus, flags;

  pstatus = find_status_by_pid (pid);

  if (pstatus == PROC_BAD)
    {
      internal_error (_("wait: pid %ld is not a child of this shell"), (long)pid);
      return (127);
    }

  if (pstatus != PROC_STILL_ALIVE)
    {
      if (pstatus > 128)
	last_command_exit_signal = find_termsig_by_pid (pid);
      return (pstatus);
    }

  siginterrupt (SIGINT, 1);
  while ((got_pid = WAITPID (pid, &status, 0)) != pid)
    {
      CHECK_TERMSIG;
      CHECK_WAIT_INTR;
      if (got_pid < 0)
	{
	  if (errno != EINTR && errno != ECHILD)
	    {
	      siginterrupt (SIGINT, 0);
	      sys_error ("wait");
	    }
	  break;
	}
      else if (got_pid > 0)
	set_pid_status (got_pid, status);
    }

  if (got_pid > 0)
    {
      set_pid_status (got_pid, status);
      set_pid_flags (got_pid, PROC_NOTIFIED);
    }

  siginterrupt (SIGINT, 0);
  QUIT;

  return (got_pid > 0 ? process_exit_status (status) : -1);
}