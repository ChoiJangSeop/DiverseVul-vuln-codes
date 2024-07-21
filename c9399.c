delete_pidfile(void)
{
  const char *pidfile = CNF_GetPidFile();

  if (!pidfile[0])
    return;

  /* Don't care if this fails, there's not a lot we can do */
  unlink(pidfile);
}