check_pidfile(void)
{
  const char *pidfile = CNF_GetPidFile();
  FILE *in;
  int pid, count;
  
  in = fopen(pidfile, "r");
  if (!in)
    return;

  count = fscanf(in, "%d", &pid);
  fclose(in);
  
  if (count != 1)
    return;

  if (getsid(pid) < 0)
    return;

  LOG_FATAL("Another chronyd may already be running (pid=%d), check %s",
            pid, pidfile);
}