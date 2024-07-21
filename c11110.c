null_asynch_zero (struct rw *rw, struct command *command,
                  nbd_completion_callback cb, bool allocate)
{
  int dummy = 0;

  if (cb.callback (cb.user_data, &dummy) == -1) {
    perror (rw->name);
    exit (EXIT_FAILURE);
  }
  return true;
}