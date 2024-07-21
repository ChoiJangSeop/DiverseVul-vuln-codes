execute_pipeline (command, asynchronous, pipe_in, pipe_out, fds_to_close)
     COMMAND *command;
     int asynchronous, pipe_in, pipe_out;
     struct fd_bitmap *fds_to_close;
{
  int prev, fildes[2], new_bitmap_size, dummyfd, ignore_return, exec_result;
  int lstdin, lastpipe_flag, lastpipe_jid;
  COMMAND *cmd;
  struct fd_bitmap *fd_bitmap;
  pid_t lastpid;

#if defined (JOB_CONTROL)
  sigset_t set, oset;
  BLOCK_CHILD (set, oset);
#endif /* JOB_CONTROL */

  ignore_return = (command->flags & CMD_IGNORE_RETURN) != 0;

  prev = pipe_in;
  cmd = command;

  while (cmd && cmd->type == cm_connection &&
	 cmd->value.Connection && cmd->value.Connection->connector == '|')
    {
      /* Make a pipeline between the two commands. */
      if (pipe (fildes) < 0)
	{
	  sys_error (_("pipe error"));
#if defined (JOB_CONTROL)
	  terminate_current_pipeline ();
	  kill_current_pipeline ();
	  UNBLOCK_CHILD (oset);
#endif /* JOB_CONTROL */
	  last_command_exit_value = EXECUTION_FAILURE;
	  /* The unwind-protects installed below will take care
	     of closing all of the open file descriptors. */
	  throw_to_top_level ();
	  return (EXECUTION_FAILURE);	/* XXX */
	}

      /* Here is a problem: with the new file close-on-exec
	 code, the read end of the pipe (fildes[0]) stays open
	 in the first process, so that process will never get a
	 SIGPIPE.  There is no way to signal the first process
	 that it should close fildes[0] after forking, so it
	 remains open.  No SIGPIPE is ever sent because there
	 is still a file descriptor open for reading connected
	 to the pipe.  We take care of that here.  This passes
	 around a bitmap of file descriptors that must be
	 closed after making a child process in execute_simple_command. */

      /* We need fd_bitmap to be at least as big as fildes[0].
	 If fildes[0] is less than fds_to_close->size, then
	 use fds_to_close->size. */
      new_bitmap_size = (fildes[0] < fds_to_close->size)
				? fds_to_close->size
				: fildes[0] + 8;

      fd_bitmap = new_fd_bitmap (new_bitmap_size);

      /* Now copy the old information into the new bitmap. */
      xbcopy ((char *)fds_to_close->bitmap, (char *)fd_bitmap->bitmap, fds_to_close->size);

      /* And mark the pipe file descriptors to be closed. */
      fd_bitmap->bitmap[fildes[0]] = 1;

      /* In case there are pipe or out-of-processes errors, we
	 want all these file descriptors to be closed when
	 unwind-protects are run, and the storage used for the
	 bitmaps freed up. */
      begin_unwind_frame ("pipe-file-descriptors");
      add_unwind_protect (dispose_fd_bitmap, fd_bitmap);
      add_unwind_protect (close_fd_bitmap, fd_bitmap);
      if (prev >= 0)
	add_unwind_protect (close, prev);
      dummyfd = fildes[1];
      add_unwind_protect (close, dummyfd);

#if defined (JOB_CONTROL)
      add_unwind_protect (restore_signal_mask, &oset);
#endif /* JOB_CONTROL */

      if (ignore_return && cmd->value.Connection->first)
	cmd->value.Connection->first->flags |= CMD_IGNORE_RETURN;
      execute_command_internal (cmd->value.Connection->first, asynchronous,
				prev, fildes[1], fd_bitmap);

      if (prev >= 0)
	close (prev);

      prev = fildes[0];
      close (fildes[1]);

      dispose_fd_bitmap (fd_bitmap);
      discard_unwind_frame ("pipe-file-descriptors");

      cmd = cmd->value.Connection->second;
    }

  lastpid = last_made_pid;

  /* Now execute the rightmost command in the pipeline.  */
  if (ignore_return && cmd)
    cmd->flags |= CMD_IGNORE_RETURN;

#if defined (JOB_CONTROL)
  lastpipe_flag = 0;
  begin_unwind_frame ("lastpipe-exec");
  lstdin = -1;
  /* If the `lastpipe' option is set with shopt, and job control is not
     enabled, execute the last element of non-async pipelines in the
     current shell environment. */
  if (lastpipe_opt && job_control == 0 && asynchronous == 0 && pipe_out == NO_PIPE && prev > 0)
    {
      lstdin = move_to_high_fd (0, 0, 255);
      if (lstdin > 0)
	{
	  do_piping (prev, pipe_out);
	  prev = NO_PIPE;
	  add_unwind_protect (restore_stdin, lstdin);
	  lastpipe_flag = 1;
	  freeze_jobs_list ();
	  lastpipe_jid = stop_pipeline (0, (COMMAND *)NULL);	/* XXX */
	  add_unwind_protect (lastpipe_cleanup, lastpipe_jid);
	}
      if (cmd)
	cmd->flags |= CMD_LASTPIPE;
    }	  
  if (prev >= 0)
    add_unwind_protect (close, prev);
#endif

  exec_result = execute_command_internal (cmd, asynchronous, prev, pipe_out, fds_to_close);

#if defined (JOB_CONTROL)
  if (lstdin > 0)
    restore_stdin (lstdin);
#endif

  if (prev >= 0)
    close (prev);

#if defined (JOB_CONTROL)
  UNBLOCK_CHILD (oset);
#endif

  QUIT;

  if (lastpipe_flag)
    {
#if defined (JOB_CONTROL)
      append_process (savestring (the_printed_command), dollar_dollar_pid, exec_result, lastpipe_jid);
#endif
      lstdin = wait_for (lastpid);
#if defined (JOB_CONTROL)
      exec_result = job_exit_status (lastpipe_jid);
#endif
      unfreeze_jobs_list ();
    }

  discard_unwind_frame ("lastpipe-exec");

  return (exec_result);
}