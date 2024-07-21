file_find(const char *path,		/* I - Path "dir;dir;dir" */
          const char *s)		/* I - File to find */
{
  int		i;			/* Looping var */
  char		*temp;			/* Current position in filename */
  const char	*sptr;			/* Pointer into "s" */
  int		ch;			/* Quoted character */
  char		basename[HTTP_MAX_URI];	/* Base (unquoted) filename */
  const char	*realname;		/* Real filename */
  static char	filename[HTTP_MAX_URI];	/* Current filename */


 /*
  * If the filename is NULL, return NULL...
  */

  if (s == NULL)
    return (NULL);

  DEBUG_printf(("file_find(path=\"%s\", s=\"%s\")\n", path ? path : "(null)", s));

 /*
  * See if this is a cached remote file...
  */

  for (i = 0; i < (int)web_files; i ++)
    if (strcmp(s, web_cache[i].name) == 0)
    {
      DEBUG_printf(("file_find: Returning cache file \"%s\"!\n", s));
      return (web_cache[i].name);
    }

  DEBUG_printf(("file_find: \"%s\" not in web cache of %d files...\n", s, (int)web_files));

 /*
  * Make sure the filename is not quoted...
  */

  if (strchr(s, '%') == NULL)
    strlcpy(basename, s, sizeof(basename));
  else
  {
    for (sptr = s, temp = basename;
	 *sptr && temp < (basename + sizeof(basename) - 1);)
      if (*sptr == '%' && isxdigit(sptr[1]) && isxdigit(sptr[2]))
      {
       /*
	* Dequote %HH...
	*/

	if (isalpha(sptr[1]))
	  ch = (tolower(sptr[1]) - 'a' + 10) << 4;
	else
	  ch = (sptr[1] - '0') << 4;

	if (isalpha(sptr[2]))
	  ch |= tolower(sptr[2]) - 'a' + 10;
	else
	  ch |= sptr[2] - '0';

	*temp++ = (char)ch;

	sptr += 3;
      }
      else
	*temp++ = *sptr++;

    *temp = '\0';
  }

 /*
  * If we got a complete URL, we don't use the path...
  */

  if (path != NULL && !path[0])
  {
    DEBUG_puts("file_find: Resetting path to NULL since path is empty...");
    path = NULL;
  }

  if (strncmp(s, "http:", 5) == 0 ||
      strncmp(s, "https:", 6) == 0 ||
      strncmp(s, "//", 2) == 0)
  {
    DEBUG_puts("file_find: Resetting path to NULL since filename is a URL...");
    path = NULL;
  }

 /*
  * Loop through the path as needed...
  */

  if (path != NULL)
  {
    filename[sizeof(filename) - 1] = '\0';

    while (*path != '\0')
    {
     /*
      * Copy the path directory...
      */

      temp = filename;

      while (*path != ';' && *path && temp < (filename + sizeof(filename) - 1))
	*temp++ = *path++;

      if (*path == ';')
	path ++;

     /*
      * Append a slash as needed, then the filename...
      */

      if (temp > filename && temp < (filename + sizeof(filename) - 1) &&
          basename[0] != '/')
	*temp++ = '/';

      strlcpy(temp, basename, sizeof(filename) - (size_t)(temp - filename));

     /*
      * See if the file or URL exists...
      */

      if ((realname = file_find_check(filename)) != NULL)
	return (realname);
    }
  }

  if (file_find_check(s))
  {
    strlcpy(filename, s, sizeof(filename));
    return (filename);
  }
  else
    return (NULL);
}