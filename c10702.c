read_pattern_file ()
{
  int max_new_patterns;
  char **new_save_patterns;
  int new_num_patterns;
  int i;
  dynamic_string pattern_name;
  FILE *pattern_fp;

  if (num_patterns < 0)
    num_patterns = 0;
  max_new_patterns = 1 + num_patterns;
  new_save_patterns = (char **) xmalloc (max_new_patterns * sizeof (char *));
  new_num_patterns = num_patterns;
  ds_init (&pattern_name, 128);

  pattern_fp = fopen (pattern_file_name, "r");
  if (pattern_fp == NULL)
    open_fatal (pattern_file_name);
  while (ds_fgetstr (pattern_fp, &pattern_name, '\n') != NULL)
    {
      if (new_num_patterns >= max_new_patterns)
	{
	  max_new_patterns += 1;
	  new_save_patterns = (char **)
	    xrealloc ((char *) new_save_patterns,
		      max_new_patterns * sizeof (char *));
	}
      new_save_patterns[new_num_patterns] = xstrdup (pattern_name.ds_string);
      ++new_num_patterns;
    }
  if (ferror (pattern_fp) || fclose (pattern_fp) == EOF)
    close_error (pattern_file_name);

  for (i = 0; i < num_patterns; ++i)
    new_save_patterns[i] = save_patterns[i];

  save_patterns = new_save_patterns;
  num_patterns = new_num_patterns;
}