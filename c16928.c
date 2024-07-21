check_selfref (name, value, flags)
     const char *name;
     const char *value;
     int flags;
{
  char *t;

  if (STREQ (name, value))
    return 1;

#if defined (ARRAY_VARS)
  if (valid_array_reference (value, 0))
    {
      t = array_variable_name (value, (int *)NULL, (int *)NULL);
      if (t && STREQ (name, t))
	{
	  free (t);
	  return 1;
	}
      free (t);
    }
#endif

  return 0;	/* not a self reference */
}