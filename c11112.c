free_command (void *vp, int *error)
{
  struct command *command = vp;
  struct buffer *buffer = command->slice.buffer;

  if (buffer != NULL) {
    if (--buffer->refs == 0) {
      free (buffer->data);
      free (buffer);
    }
  }

  free (command);

  return 1; /* auto-retires the command */
}