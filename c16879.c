compileFile (const char *fileName)
{
/*Compile a table file */
  FileInfo nested;
  fileCount++;
  nested.fileName = fileName;
  nested.encoding = noEncoding;
  nested.status = 0;
  nested.lineNumber = 0;
  if ((nested.in = findTable (fileName)))
    {
      while (getALine (&nested))
	compileRule (&nested);
      fclose (nested.in);
    }
  else
    {
      if (fileCount > 1)
	lou_logPrint ("Cannot open table '%s'", nested.fileName);
      errorCount++;
      return 0;
    }
  return 1;
}