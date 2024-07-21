LOG_OpenFileLog(const char *log_file)
{
  FILE *f;

  if (log_file) {
    f = fopen(log_file, "a");
    if (!f)
      LOG_FATAL("Could not open log file %s", log_file);
  } else {
    f = stderr;
  }

  /* Enable line buffering */
  setvbuf(f, NULL, _IOLBF, BUFSIZ);

  if (file_log && file_log != stderr)
    fclose(file_log);

  file_log = f;
}