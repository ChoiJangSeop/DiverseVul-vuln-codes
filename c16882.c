lou_getTable (const char *tableList)
{
/* Search paths for tables and keep track of compiled tables. */
  void *table = NULL;
  char *pathList;
  char pathEnd[2];
  char trialPath[MAXSTRING];
  if (tableList == NULL || tableList[0] == 0)
    return NULL;
  errorCount = fileCount = 0;
  pathEnd[0] = DIR_SEP;
  pathEnd[1] = 0;
  /* See if table is on environment path LOUIS_TABLEPATH */
  pathList = getenv ("LOUIS_TABLEPATH");
  if (pathList)
    while (1)
      {
	int k;
	int listLength;
	int currentListPos = 0;
	listLength = strlen (pathList);
	for (k = 0; k < listLength; k++)
	  if (pathList[k] == ',')
	    break;
	if (k == listLength || k == 0)
	  {			/* Only one file */
	    strcpy (trialPath, pathList);
	    strcat (trialPath, pathEnd);
	    strcat (trialPath, tableList);
	    table = getTable (trialPath);
	    if (table)
	      break;
	  }
	else
	  {			/* Compile a list of files */
	    strncpy (trialPath, pathList, k);
	    trialPath[k] = 0;
	    strcat (trialPath, pathEnd);
	    strcat (trialPath, tableList);
	    currentListPos = k + 1;
	    table = getTable (trialPath);
	    if (table)
	      break;
	    while (currentListPos < listLength)
	      {
		for (k = currentListPos; k < listLength; k++)
		  if (pathList[k] == ',')
		    break;
		strncpy (trialPath,
			 &pathList[currentListPos], k - currentListPos);
		trialPath[k - currentListPos] = 0;
		strcat (trialPath, pathEnd);
		strcat (trialPath, tableList);
		table = getTable (trialPath);
		currentListPos = k + 1;
		if (table)
		  break;
	      }
	  }
	break;
      }
  if (!table)
    {
      /* See if table in current directory or on a path in 
       * the table name*/
      if (errorCount > 0 && (!(errorCount == 1 && fileCount == 1)))
	return NULL;
      table = getTable (tableList);
    }
  if (!table)
    {
/* See if table on dataPath. */
      if (errorCount > 0 && (!(errorCount == 1 && fileCount == 1)))
	return NULL;
      pathList = lou_getDataPath ();
      if (pathList)
	{
	  strcpy (trialPath, pathList);
	  strcat (trialPath, pathEnd);
#ifdef _WIN32
	  strcat (trialPath, "liblouis\\tables\\");
#else
	  strcat (trialPath, "liblouis/tables/");
#endif
	  strcat (trialPath, tableList);
	  table = getTable (trialPath);
	}
    }
  if (!table)
    {
      /* See if table on installed or program path. */
      if (errorCount > 0 && (!(errorCount == 1 && fileCount == 1)))
	return NULL;
#ifdef _WIN32
      strcpy (trialPath, lou_getProgramPath ());
      strcat (trialPath, "\\share\\liblouss\\tables\\");
#else
      strcpy (trialPath, TABLESDIR);
      strcat (trialPath, pathEnd);
#endif
      strcat (trialPath, tableList);
      table = getTable (trialPath);
    }
  if (!table)
    lou_logPrint ("%s could not be found", tableList);
  return table;
}