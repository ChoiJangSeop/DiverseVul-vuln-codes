bash_directory_completion_hook (dirname)
     char **dirname;
{
  char *local_dirname, *new_dirname, *t;
  int return_value, should_expand_dirname, nextch, closer, changed;
  size_t local_dirlen;
  WORD_LIST *wl;
  struct stat sb;

  return_value = should_expand_dirname = nextch = closer = 0;
  local_dirname = *dirname;

  if (t = mbschr (local_dirname, '$'))
    {
      should_expand_dirname = '$';
      nextch = t[1];
      /* Deliberately does not handle the deprecated $[...] arithmetic
	 expansion syntax */
      if (nextch == '(')
	closer = ')';
      else if (nextch == '{')
	closer = '}';
      else
	nextch = 0;
    }
  else if (local_dirname[0] == '~')
    should_expand_dirname = '~';
  else
    {
      t = mbschr (local_dirname, '`');
      if (t && unclosed_pair (local_dirname, strlen (local_dirname), "`") == 0)
	should_expand_dirname = '`';
    }

  if (should_expand_dirname && directory_exists (local_dirname))
    should_expand_dirname = 0;

  if (should_expand_dirname)  
    {
      new_dirname = savestring (local_dirname);
      wl = expand_prompt_string (new_dirname, 0, W_NOCOMSUB|W_COMPLETE);	/* does the right thing */
      if (wl)
	{
	  *dirname = string_list (wl);
	  /* Tell the completer to replace the directory name only if we
	     actually expanded something. */
	  return_value = STREQ (local_dirname, *dirname) == 0;
	  free (local_dirname);
	  free (new_dirname);
	  dispose_words (wl);
	  local_dirname = *dirname;
	  /* XXX - change rl_filename_quote_characters here based on
	     should_expand_dirname/nextch/closer.  This is the only place
	     custom_filename_quote_characters is modified. */
	  if (rl_filename_quote_characters && *rl_filename_quote_characters)
	    {
	      int i, j, c;
	      i = strlen (default_filename_quote_characters);
	      custom_filename_quote_characters = xrealloc (custom_filename_quote_characters, i+1);
	      for (i = j = 0; c = default_filename_quote_characters[i]; i++)
		{
		  if (c == should_expand_dirname || c == nextch || c == closer)
		    continue;
		  custom_filename_quote_characters[j++] = c;
		}
	      custom_filename_quote_characters[j] = '\0';
	      rl_filename_quote_characters = custom_filename_quote_characters;
	      set_filename_bstab (rl_filename_quote_characters);
	    }
	}
      else
	{
	  free (new_dirname);
	  free (local_dirname);
	  *dirname = (char *)xmalloc (1);
	  **dirname = '\0';
	  return 1;
	}
    }
  else 
    {
      /* Dequote the filename even if we don't expand it. */
      new_dirname = bash_dequote_filename (local_dirname, rl_completion_quote_character);
      return_value = STREQ (local_dirname, new_dirname) == 0;
      free (local_dirname);
      local_dirname = *dirname = new_dirname;
    }

  /* no_symbolic_links == 0 -> use (default) logical view of the file system.
     local_dirname[0] == '.' && local_dirname[1] == '/' means files in the
     current directory (./).
     local_dirname[0] == '.' && local_dirname[1] == 0 means relative pathnames
     in the current directory (e.g., lib/sh).
     XXX - should we do spelling correction on these? */

  /* This is test as it was in bash-4.2: skip relative pathnames in current
     directory.  Change test to
      (local_dirname[0] != '.' || (local_dirname[1] && local_dirname[1] != '/'))
     if we want to skip paths beginning with ./ also. */
  if (no_symbolic_links == 0 && (local_dirname[0] != '.' || local_dirname[1]))
    {
      char *temp1, *temp2;
      int len1, len2;

      /* If we have a relative path
      		(local_dirname[0] != '/' && local_dirname[0] != '.')
	 that is canonical after appending it to the current directory, then
	 	temp1 = temp2+'/'
	 That is,
	 	strcmp (temp1, temp2) == 0
	 after adding a slash to temp2 below.  It should be safe to not
	 change those.
      */
      t = get_working_directory ("symlink-hook");
      temp1 = make_absolute (local_dirname, t);
      free (t);
      temp2 = sh_canonpath (temp1, PATH_CHECKDOTDOT|PATH_CHECKEXISTS);

      /* Try spelling correction if initial canonicalization fails.  Make
	 sure we are set to replace the directory name with the results so
	 subsequent directory checks don't fail. */
      if (temp2 == 0 && dircomplete_spelling && dircomplete_expand)
	{
	  temp2 = dirspell (temp1);
	  if (temp2)
	    {
	      free (temp1);
	      temp1 = temp2;
	      temp2 = sh_canonpath (temp1, PATH_CHECKDOTDOT|PATH_CHECKEXISTS);
	      return_value |= temp2 != 0;
	    }
	}
      /* If we can't canonicalize, bail. */
      if (temp2 == 0)
	{
	  free (temp1);
	  return return_value;
	}
      len1 = strlen (temp1);
      if (temp1[len1 - 1] == '/')
	{
	  len2 = strlen (temp2);
	  if (len2 > 2)		/* don't append `/' to `/' or `//' */
	    {
	      temp2 = (char *)xrealloc (temp2, len2 + 2);
	      temp2[len2] = '/';
	      temp2[len2 + 1] = '\0';
	    }
	}

      /* dircomplete_expand_relpath == 0 means we want to leave relative
	 pathnames that are unchanged by canonicalization alone.
	 *local_dirname != '/' && *local_dirname != '.' == relative pathname
	 (consistent with general.c:absolute_pathname())
	 temp1 == temp2 (after appending a slash to temp2) means the pathname
	 is not changed by canonicalization as described above. */
      if (dircomplete_expand_relpath || ((local_dirname[0] != '/' && local_dirname[0] != '.') && STREQ (temp1, temp2) == 0))
	return_value |= STREQ (local_dirname, temp2) == 0;
      free (local_dirname);
      *dirname = temp2;
      free (temp1);
    }

  return (return_value);
}