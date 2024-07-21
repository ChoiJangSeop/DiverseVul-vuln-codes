read_cups_files_conf(cups_file_t *fp)	/* I - File to read from */
{
  int		linenum;		/* Current line number */
  char		line[HTTP_MAX_BUFFER],	/* Line from file */
		*value;			/* Value from line */
  struct group	*group;			/* Group */


 /*
  * Loop through each line in the file...
  */

  linenum = 0;

  while (cupsFileGetConf(fp, line, sizeof(line), &value, &linenum))
  {
    if (!_cups_strcasecmp(line, "FatalErrors"))
      FatalErrors = parse_fatal_errors(value);
    else if (!_cups_strcasecmp(line, "Group") && value)
    {
     /*
      * Group ID to run as...
      */

      if (isdigit(value[0]))
        Group = (gid_t)atoi(value);
      else
      {
        endgrent();
	group = getgrnam(value);

	if (group != NULL)
	  Group = group->gr_gid;
	else
	{
	  cupsdLogMessage(CUPSD_LOG_ERROR,
	                  "Unknown Group \"%s\" on line %d of %s.", value,
	                  linenum, CupsFilesFile);
	  if (FatalErrors & CUPSD_FATAL_CONFIG)
	    return (0);
	}
      }
    }
    else if (!_cups_strcasecmp(line, "PrintcapFormat") && value)
    {
     /*
      * Format of printcap file?
      */

      if (!_cups_strcasecmp(value, "bsd"))
        PrintcapFormat = PRINTCAP_BSD;
      else if (!_cups_strcasecmp(value, "plist"))
        PrintcapFormat = PRINTCAP_PLIST;
      else if (!_cups_strcasecmp(value, "solaris"))
        PrintcapFormat = PRINTCAP_SOLARIS;
      else
      {
	cupsdLogMessage(CUPSD_LOG_ERROR,
	                "Unknown PrintcapFormat \"%s\" on line %d of %s.",
	                value, linenum, CupsFilesFile);
        if (FatalErrors & CUPSD_FATAL_CONFIG)
          return (0);
      }
    }
    else if (!_cups_strcasecmp(line, "Sandboxing") && value)
    {
     /*
      * Level of sandboxing?
      */

      if (!_cups_strcasecmp(value, "off") && getuid())
      {
        Sandboxing = CUPSD_SANDBOXING_OFF;
        cupsdLogMessage(CUPSD_LOG_WARN, "Disabling sandboxing is not recommended (line %d of %s)", linenum, CupsFilesFile);
      }
      else if (!_cups_strcasecmp(value, "relaxed"))
        Sandboxing = CUPSD_SANDBOXING_RELAXED;
      else if (!_cups_strcasecmp(value, "strict"))
        Sandboxing = CUPSD_SANDBOXING_STRICT;
      else
      {
	cupsdLogMessage(CUPSD_LOG_ERROR,
	                "Unknown Sandboxing \"%s\" on line %d of %s.",
	                value, linenum, CupsFilesFile);
        if (FatalErrors & CUPSD_FATAL_CONFIG)
          return (0);
      }
    }
    else if (!_cups_strcasecmp(line, "SystemGroup") && value)
    {
     /*
      * SystemGroup (admin) group(s)...
      */

      if (!parse_groups(value, linenum))
      {
        if (FatalErrors & CUPSD_FATAL_CONFIG)
          return (0);
      }
    }
    else if (!_cups_strcasecmp(line, "User") && value)
    {
     /*
      * User ID to run as...
      */

      if (isdigit(value[0] & 255))
      {
        int uid = atoi(value);

	if (!uid)
	{
	  cupsdLogMessage(CUPSD_LOG_ERROR,
	                  "Will not use User 0 as specified on line %d of %s "
			  "for security reasons.  You must use a non-"
			  "privileged account instead.",
	                  linenum, CupsFilesFile);
          if (FatalErrors & CUPSD_FATAL_CONFIG)
            return (0);
        }
        else
	  User = (uid_t)atoi(value);
      }
      else
      {
        struct passwd *p;	/* Password information */

        endpwent();
	p = getpwnam(value);

	if (p)
	{
	  if (!p->pw_uid)
	  {
	    cupsdLogMessage(CUPSD_LOG_ERROR,
	                    "Will not use User %s (UID=0) as specified on line "
			    "%d of %s for security reasons.  You must use a "
			    "non-privileged account instead.",
	                    value, linenum, CupsFilesFile);
	    if (FatalErrors & CUPSD_FATAL_CONFIG)
	      return (0);
	  }
	  else
	    User = p->pw_uid;
	}
	else
	{
	  cupsdLogMessage(CUPSD_LOG_ERROR,
	                  "Unknown User \"%s\" on line %d of %s.",
	                  value, linenum, CupsFilesFile);
          if (FatalErrors & CUPSD_FATAL_CONFIG)
            return (0);
        }
      }
    }
    else if (!_cups_strcasecmp(line, "ServerCertificate") ||
             !_cups_strcasecmp(line, "ServerKey"))
    {
      cupsdLogMessage(CUPSD_LOG_INFO,
		      "The \"%s\" directive on line %d of %s is no longer "
		      "supported; this will become an error in a future "
		      "release.",
		      line, linenum, CupsFilesFile);
    }
    else if (!parse_variable(CupsFilesFile, linenum, line, value,
			     sizeof(cupsfiles_vars) / sizeof(cupsfiles_vars[0]),
			     cupsfiles_vars) &&
	     (FatalErrors & CUPSD_FATAL_CONFIG))
      return (0);
  }

  return (1);
}