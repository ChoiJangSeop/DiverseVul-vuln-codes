write_pidfile(void)
{
  const char *pidfile = CNF_GetPidFile();
  FILE *out;

  if (!pidfile[0])
    return;

  out = fopen(pidfile, "w");
  if (!out) {
    LOG_FATAL("Could not open %s : %s", pidfile, strerror(errno));
  } else {
    fprintf(out, "%d\n", (int)getpid());
    fclose(out);
  }
}