doit (void)
{
  char *p;
  int rc;
  size_t i;

  if (!stringprep_check_version (STRINGPREP_VERSION))
    fail ("stringprep_check_version failed (header %s runtime %s)\n",
	  STRINGPREP_VERSION, stringprep_check_version (NULL));

  if (!stringprep_check_version (NULL))
    fail ("stringprep_check_version(NULL) failed\n");

  if (strcmp (stringprep_check_version (NULL), STRINGPREP_VERSION) != 0)
    fail ("stringprep_check_version failure (header %s runtime %s)\n",
	  STRINGPREP_VERSION, stringprep_check_version (NULL));

  if (stringprep_check_version ("100.100"))
    fail ("stringprep_check_version(\"100.100\") failed\n");

  for (i = 0; i < sizeof (strprep) / sizeof (strprep[0]); i++)
    {
      if (debug)
	printf ("STRINGPREP entry %ld\n", i);

      if (debug)
	{
	  printf ("flags: %d\n", strprep[i].flags);

	  printf ("in: ");
	  escapeprint (strprep[i].in, strlen (strprep[i].in));
	  hexprint (strprep[i].in, strlen (strprep[i].in));
	  binprint (strprep[i].in, strlen (strprep[i].in));
	}

      {
	uint32_t *l;
	char *x;
	l = stringprep_utf8_to_ucs4 (strprep[i].in, -1, NULL);
	x = stringprep_ucs4_to_utf8 (l, -1, NULL, NULL);
	free (l);

	if (strcmp (strprep[i].in, x) != 0)
	  {
	    fail ("bad UTF-8 in entry %ld\n", i);
	    if (debug)
	      {
		puts ("expected:");
		escapeprint (strprep[i].in, strlen (strprep[i].in));
		hexprint (strprep[i].in, strlen (strprep[i].in));
		puts ("computed:");
		escapeprint (x, strlen (x));
		hexprint (x, strlen (x));
	      }
	  }

	free (x);
      }
      rc = stringprep_profile (strprep[i].in, &p,
			       strprep[i].profile ?
			       strprep[i].profile :
			       "Nameprep", strprep[i].flags);
      if (rc != strprep[i].rc)
	{
	  fail ("stringprep() entry %ld failed: %d\n", i, rc);
	  if (debug)
	    printf ("FATAL\n");
	  if (rc == STRINGPREP_OK)
	    free (p);
	  continue;
	}

      if (debug && rc == STRINGPREP_OK)
	{
	  printf ("out: ");
	  escapeprint (p, strlen (p));
	  hexprint (p, strlen (p));
	  binprint (p, strlen (p));

	  printf ("expected out: ");
	  escapeprint (strprep[i].out, strlen (strprep[i].out));
	  hexprint (strprep[i].out, strlen (strprep[i].out));
	  binprint (strprep[i].out, strlen (strprep[i].out));
	}
      else if (debug)
	printf ("returned %d expected %d\n", rc, strprep[i].rc);

      if (rc == STRINGPREP_OK)
	{
	  if (strlen (strprep[i].out) != strlen (p) ||
	      memcmp (strprep[i].out, p, strlen (p)) != 0)
	    {
	      fail ("stringprep() entry %ld failed\n", i);
	      if (debug)
		printf ("ERROR\n");
	    }
	  else if (debug)
	    printf ("OK\n\n");

	  free (p);
	}
      else if (debug)
	printf ("OK\n\n");
    }

#if 0
  {
    char p[20];
    memset (p, 0, 10);
    stringprep_unichar_to_utf8 (0x00DF, p);
    hexprint (p, strlen (p));
    puts ("");
  }
#endif
}