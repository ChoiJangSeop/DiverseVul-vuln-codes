file_asynch_write (struct rw *rw,
                   struct command *command,
                   nbd_completion_callback cb)
{
  int dummy = 0;

  file_synch_write (rw, slice_ptr (command->slice),
                    command->slice.len, command->offset);
  if (cb.callback (cb.user_data, &dummy) == -1) {
    perror (rw->name);
    exit (EXIT_FAILURE);
  }
}