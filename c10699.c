query_rename(struct cpio_file_stat* file_hdr, FILE *tty_in, FILE *tty_out,
	     FILE *rename_in)
{
  char *str_res;		/* Result for string function.  */
  static dynamic_string new_name;	/* New file name for rename option.  */
  static int initialized_new_name = false;
  if (!initialized_new_name)
  {
    ds_init (&new_name, 128);
    initialized_new_name = true;
  }

  if (rename_flag)
    {
      fprintf (tty_out, _("rename %s -> "), file_hdr->c_name);
      fflush (tty_out);
      str_res = ds_fgets (tty_in, &new_name);
    }
  else
    {
      str_res = ds_fgetstr (rename_in, &new_name, '\n');
    }
  if (str_res == NULL || str_res[0] == 0)
    {
      return -1;
    }
  else
    cpio_set_c_name (file_hdr, new_name.ds_string);
  return 0;
}