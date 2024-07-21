null_asynch_write (struct rw *rw,
                   struct command *command,
                   nbd_completion_callback cb)
{
  int dummy = 0;

  if (cb.callback (cb.user_data, &dummy) == -1) {
    perror (rw->name);
    exit (EXIT_FAILURE);
  }
}