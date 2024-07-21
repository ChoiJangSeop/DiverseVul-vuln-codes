compileTranslationTable (const char *tableList)
{
/*compile source tables into a table in memory */
  int k;
  char mainTable[MAXSTRING];
  char subTable[MAXSTRING];
  int listLength;
  int currentListPos = 0;
  errorCount = 0;
  warningCount = 0;
  fileCount = 0;
  table = NULL;
  characterClasses = NULL;
  ruleNames = NULL;
  if (tableList == NULL)
    return NULL;
  if (!opcodeLengths[0])
    {
      TranslationTableOpcode opcode;
      for (opcode = 0; opcode < CTO_None; opcode++)
	opcodeLengths[opcode] = strlen (opcodeNames[opcode]);
    }
  allocateHeader (NULL);
  /*Compile things that are necesary for the proper operation of 
     liblouis or liblouisxml or liblouisutdml */
  compileString ("space \\s 0");
  compileString ("noback sign \\x0000 0");
  compileString ("space \\x00a0 a unbreakable space");
  compileString ("space \\x001b 1b escape");
  compileString ("space \\xffff 123456789abcdef ENDSEGMENT");
  listLength = strlen (tableList);
  for (k = currentListPos; k < listLength; k++)
    if (tableList[k] == ',')
      break;
  if (k == listLength)
    {				/* Only one file */
      strcpy (tablePath, tableList);
      for (k = strlen (tablePath); k >= 0; k--)
	if (tablePath[k] == '\\' || tablePath[k] == '/')
	  break;
      strcpy (mainTable, &tablePath[k + 1]);
      tablePath[++k] = 0;
      if (!compileFile (mainTable))
	goto cleanup;
    }
  else
    {				/* Compile a list of files */
      currentListPos = k + 1;
      strncpy (tablePath, tableList, k);
      tablePath[k] = 0;
      for (k = strlen (tablePath); k >= 0; k--)
	if (tablePath[k] == '\\' || tablePath[k] == '/')
	  break;
      strcpy (mainTable, &tablePath[k + 1]);
      tablePath[++k] = 0;
      if (!compileFile (mainTable))
	goto cleanup;
      while (currentListPos < listLength)
	{
	  for (k = currentListPos; k < listLength; k++)
	    if (tableList[k] == ',')
	      break;
	  strncpy (subTable, &tableList[currentListPos], k - currentListPos);
	  subTable[k - currentListPos] = 0;
	  if (!compileFile (subTable))
	    goto cleanup;
	  currentListPos = k + 1;
	}
    }
/*Clean up after compiling files*/
cleanup:
  if (characterClasses)
    deallocateCharacterClasses ();
  if (ruleNames)
    deallocateRuleNames ();
  if (warningCount)
    lou_logPrint ("%d warnings issued", warningCount);
  if (!errorCount)
    {
      setDefaults ();
      table->tableSize = tableSize;
      table->bytesUsed = tableUsed;
    }
  else
    {
      if (!(errorCount == 1 && fileCount == 1))
	lou_logPrint ("%d errors found.", errorCount);
      if (table)
	free (table);
      table = NULL;
    }
  return (void *) table;
}