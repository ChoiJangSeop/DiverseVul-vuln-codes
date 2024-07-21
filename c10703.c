get_next_reel (int tape_des)
{
  static int reel_number = 1;
  FILE *tty_in;			/* File for interacting with user.  */
  FILE *tty_out;		/* File for interacting with user.  */
  int old_tape_des;
  char *next_archive_name;
  dynamic_string new_name;
  char *str_res;

  ds_init (&new_name, 128);

  /* Open files for interactive communication.  */
  tty_in = fopen (TTY_NAME, "r");
  if (tty_in == NULL)
    error (PAXEXIT_FAILURE, errno, TTY_NAME);
  tty_out = fopen (TTY_NAME, "w");
  if (tty_out == NULL)
    error (PAXEXIT_FAILURE, errno, TTY_NAME);

  old_tape_des = tape_des;
  tape_offline (tape_des);
  rmtclose (tape_des);

  /* Give message and wait for carrage return.  User should hit carrage return
     only after loading the next tape.  */
  ++reel_number;
  if (new_media_message)
    fprintf (tty_out, "%s", new_media_message);
  else if (new_media_message_with_number)
    fprintf (tty_out, "%s%d%s", new_media_message_with_number, reel_number,
	     new_media_message_after_number);
  else if (archive_name)
    fprintf (tty_out, _("Found end of tape.  Load next tape and press RETURN. "));
  else
    fprintf (tty_out, _("Found end of tape.  To continue, type device/file name when ready.\n"));

  fflush (tty_out);

  if (archive_name)
    {
      int c;

      do
	c = getc (tty_in);
      while (c != EOF && c != '\n');

      tape_des = open_archive (archive_name);
      if (tape_des == -1)
	open_error (archive_name);
    }
  else
    {
      do
	{
	  if (tape_des < 0)
	    {
	      fprintf (tty_out,
		       _("To continue, type device/file name when ready.\n"));
	      fflush (tty_out);
	    }

	  str_res = ds_fgets (tty_in, &new_name);
	  if (str_res == NULL || str_res[0] == '\0')
	    exit (PAXEXIT_FAILURE);
	  next_archive_name = str_res;

	  tape_des = open_archive (next_archive_name);
	  if (tape_des == -1)
	    open_error (next_archive_name);
	}
      while (tape_des < 0);
    }

  /* We have to make sure that `tape_des' has not changed its value even
     though we closed it and reopened it, since there are local
     copies of it in other routines.  This works fine on Unix (even with
     rmtread and rmtwrite) since open will always return the lowest
     available file descriptor and we haven't closed any files (e.g.,
     stdin, stdout or stderr) that were opened before we originally opened
     the archive.  */

  if (tape_des != old_tape_des)
    error (PAXEXIT_FAILURE, 0, _("internal error: tape descriptor changed from %d to %d"),
	   old_tape_des, tape_des);

  free (new_name.ds_string);
  fclose (tty_in);
  fclose (tty_out);
}