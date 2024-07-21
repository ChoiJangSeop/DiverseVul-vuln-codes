_g_locale_get_charset_aliases (void)
{
  const char *cp;

  cp = charset_aliases;
  if (cp == NULL)
    {
#if !(defined VMS || defined WIN32_NATIVE || defined __CYGWIN__)
      FILE *fp;
      const char *dir;
      const char *base = "charset.alias";
      char *file_name;

      /* Make it possible to override the charset.alias location.  This is
	 necessary for running the testsuite before "make install".  */
      dir = getenv ("CHARSETALIASDIR");
      if (dir == NULL || dir[0] == '\0')
	dir = relocate (GLIB_CHARSETALIAS_DIR);

      /* Concatenate dir and base into freshly allocated file_name.  */
      {
	size_t dir_len = strlen (dir);
	size_t base_len = strlen (base);
	int add_slash = (dir_len > 0 && !ISSLASH (dir[dir_len - 1]));
	file_name = (char *) malloc (dir_len + add_slash + base_len + 1);
	if (file_name != NULL)
	  {
	    memcpy (file_name, dir, dir_len);
	    if (add_slash)
	      file_name[dir_len] = DIRECTORY_SEPARATOR;
	    memcpy (file_name + dir_len + add_slash, base, base_len + 1);
	  }
      }

      if (file_name == NULL || (fp = fopen (file_name, "r")) == NULL)
	/* Out of memory or file not found, treat it as empty.  */
	cp = "";
      else
	{
	  /* Parse the file's contents.  */
	  char *res_ptr = NULL;
	  size_t res_size = 0;

	  for (;;)
	    {
	      int c;
	      char buf1[50+1];
	      char buf2[50+1];
	      size_t l1, l2;
	      char *old_res_ptr;

	      c = getc (fp);
	      if (c == EOF)
		break;
	      if (c == '\n' || c == ' ' || c == '\t')
		continue;
	      if (c == '#')
		{
		  /* Skip comment, to end of line.  */
		  do
		    c = getc (fp);
		  while (!(c == EOF || c == '\n'));
		  if (c == EOF)
		    break;
		  continue;
		}
	      ungetc (c, fp);
	      if (fscanf (fp, "%50s %50s", buf1, buf2) < 2)
		break;
	      l1 = strlen (buf1);
	      l2 = strlen (buf2);
	      old_res_ptr = res_ptr;
	      if (res_size == 0)
		{
		  res_size = l1 + 1 + l2 + 1;
		  res_ptr = (char *) malloc (res_size + 1);
		}
	      else
		{
		  res_size += l1 + 1 + l2 + 1;
		  res_ptr = (char *) realloc (res_ptr, res_size + 1);
		}
	      if (res_ptr == NULL)
		{
		  /* Out of memory. */
		  res_size = 0;
		  if (old_res_ptr != NULL)
		    free (old_res_ptr);
		  break;
		}
	      strcpy (res_ptr + res_size - (l2 + 1) - (l1 + 1), buf1);
	      strcpy (res_ptr + res_size - (l2 + 1), buf2);
	    }
	  fclose (fp);
	  if (res_size == 0)
	    cp = "";
	  else
	    {
	      *(res_ptr + res_size) = '\0';
	      cp = res_ptr;
	    }
	}

      if (file_name != NULL)
	free (file_name);

#else

# if defined VMS
      /* To avoid the troubles of an extra file charset.alias_vms in the
	 sources of many GNU packages, simply inline the aliases here.  */
      /* The list of encodings is taken from the OpenVMS 7.3-1 documentation
	 "Compaq C Run-Time Library Reference Manual for OpenVMS systems"
	 section 10.7 "Handling Different Character Sets".  */
      cp = "ISO8859-1" "\0" "ISO-8859-1" "\0"
	   "ISO8859-2" "\0" "ISO-8859-2" "\0"
	   "ISO8859-5" "\0" "ISO-8859-5" "\0"
	   "ISO8859-7" "\0" "ISO-8859-7" "\0"
	   "ISO8859-8" "\0" "ISO-8859-8" "\0"
	   "ISO8859-9" "\0" "ISO-8859-9" "\0"
	   /* Japanese */
	   "eucJP" "\0" "EUC-JP" "\0"
	   "SJIS" "\0" "SHIFT_JIS" "\0"
	   "DECKANJI" "\0" "DEC-KANJI" "\0"
	   "SDECKANJI" "\0" "EUC-JP" "\0"
	   /* Chinese */
	   "eucTW" "\0" "EUC-TW" "\0"
	   "DECHANYU" "\0" "DEC-HANYU" "\0"
	   "DECHANZI" "\0" "GB2312" "\0"
	   /* Korean */
	   "DECKOREAN" "\0" "EUC-KR" "\0";
# endif

# if defined WIN32_NATIVE || defined __CYGWIN__
      /* To avoid the troubles of installing a separate file in the same
	 directory as the DLL and of retrieving the DLL's directory at
	 runtime, simply inline the aliases here.  */

      cp = "CP936" "\0" "GBK" "\0"
	   "CP1361" "\0" "JOHAB" "\0"
	   "CP20127" "\0" "ASCII" "\0"
	   "CP20866" "\0" "KOI8-R" "\0"
	   "CP20936" "\0" "GB2312" "\0"
	   "CP21866" "\0" "KOI8-RU" "\0"
	   "CP28591" "\0" "ISO-8859-1" "\0"
	   "CP28592" "\0" "ISO-8859-2" "\0"
	   "CP28593" "\0" "ISO-8859-3" "\0"
	   "CP28594" "\0" "ISO-8859-4" "\0"
	   "CP28595" "\0" "ISO-8859-5" "\0"
	   "CP28596" "\0" "ISO-8859-6" "\0"
	   "CP28597" "\0" "ISO-8859-7" "\0"
	   "CP28598" "\0" "ISO-8859-8" "\0"
	   "CP28599" "\0" "ISO-8859-9" "\0"
	   "CP28605" "\0" "ISO-8859-15" "\0"
	   "CP38598" "\0" "ISO-8859-8" "\0"
	   "CP51932" "\0" "EUC-JP" "\0"
	   "CP51936" "\0" "GB2312" "\0"
	   "CP51949" "\0" "EUC-KR" "\0"
	   "CP51950" "\0" "EUC-TW" "\0"
	   "CP54936" "\0" "GB18030" "\0"
	   "CP65001" "\0" "UTF-8" "\0";
# endif
#endif

      charset_aliases = cp;
    }

  return cp;
}