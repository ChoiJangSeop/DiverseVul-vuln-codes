findTable (const char *tableName)
{
/* Search paths for tables */
  FILE *tableFile;
  char *pathList;
  char pathEnd[2];
  char trialPath[MAXSTRING];
  if (tableName == NULL || tableName[0] == 0)
    return NULL;
  strcpy (trialPath, tablePath);
  strcat (trialPath, tableName);
  if ((tableFile = fopen (trialPath, "rb")))
    return tableFile;
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
	    strcat (trialPath, tableName);
	    if ((tableFile = fopen (trialPath, "rb")))
	      break;
	  }
	else
	  {			/* Compile a list of files */
	    strncpy (trialPath, pathList, k);
	    trialPath[k] = 0;
	    strcat (trialPath, pathEnd);
	    strcat (trialPath, tableName);
	    currentListPos = k + 1;
	    if ((tableFile = fopen (trialPath, "rb")))
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
		strcat (trialPath, tableName);
		if ((tableFile = fopen (trialPath, "rb")))
		  currentListPos = k + 1;
		break;
	      }
	  }
	break;
      }
  if (tableFile)
    return tableFile;
  /* See if table in current directory or on a path in 
   * the table name*/
  if ((tableFile = fopen (tableName, "rb")))
    return tableFile;
/* See if table on dataPath. */
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
      strcat (trialPath, tableName);
      if ((tableFile = fopen (trialPath, "rb")))
	return tableFile;
    }
  /* See if table on installed or program path. */
#ifdef _WIN32
  strcpy (trialPath, lou_getProgramPath ());
  strcat (trialPath, "\\share\\liblouis\\tables\\");
#else
  strcpy (trialPath, TABLESDIR);
  strcat (trialPath, pathEnd);
#endif
  strcat (trialPath, tableName);
  if ((tableFile = fopen (trialPath, "rb")))
    return tableFile;
  return NULL;
}