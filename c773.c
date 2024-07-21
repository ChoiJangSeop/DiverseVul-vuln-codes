main(int argc, char** argv)
{
  int ret = 0;
  int chansrv_pid = 0;
  int wm_pid = 0;
  int x_pid = 0;
  int lerror = 0;
  char exe_path[262];

  g_init("xrdp-sessvc");
  g_memset(exe_path,0,sizeof(exe_path));

  if (argc < 3)
  {
    g_writeln("xrdp-sessvc: exiting, not enough parameters");
    return 1;
  }
  g_signal_kill(term_signal_handler); /* SIGKILL */
  g_signal_terminate(term_signal_handler); /* SIGTERM */
  g_signal_user_interrupt(term_signal_handler); /* SIGINT */
  g_signal_pipe(nil_signal_handler); /* SIGPIPE */
  x_pid = g_atoi(argv[1]);
  wm_pid = g_atoi(argv[2]);
  g_writeln("xrdp-sessvc: waiting for X (pid %d) and WM (pid %d)",
             x_pid, wm_pid);
  /* run xrdp-chansrv as a seperate process */
  chansrv_pid = g_fork();
  if (chansrv_pid == -1)
  {
    g_writeln("xrdp-sessvc: fork error");
    return 1;
  }
  else if (chansrv_pid == 0) /* child */
  {
    g_set_current_dir(XRDP_SBIN_PATH);
    g_snprintf(exe_path, 261, "%s/xrdp-chansrv", XRDP_SBIN_PATH);
    g_execlp3(exe_path, "xrdp-chansrv", 0);
    /* should not get here */
    g_writeln("xrdp-sessvc: g_execlp3() failed");
    return 1;
  }
  lerror = 0;
  /* wait for window manager to get done */
  ret = g_waitpid(wm_pid);
  while ((ret == 0) && !g_term)
  {
    ret = g_waitpid(wm_pid);
    g_sleep(1);
  }
  if (ret < 0)
  {
    lerror = g_get_errno();
  }
  g_writeln("xrdp-sessvc: WM is dead (waitpid said %d, errno is %d) "
            "exiting...", ret, lerror);
  /* kill channel server */
  g_writeln("xrdp-sessvc: stopping channel server");
  g_sigterm(chansrv_pid);
  ret = g_waitpid(chansrv_pid);
  while ((ret == 0) && !g_term)
  {
    ret = g_waitpid(chansrv_pid);
    g_sleep(1);
  }
  chansrv_cleanup(chansrv_pid);
  /* kill X server */
  g_writeln("xrdp-sessvc: stopping X server");
  g_sigterm(x_pid);
  ret = g_waitpid(x_pid);
  while ((ret == 0) && !g_term)
  {
    ret = g_waitpid(x_pid);
    g_sleep(1);
  }
  g_writeln("xrdp-sessvc: clean exit");
  g_deinit();
  return 0;
}