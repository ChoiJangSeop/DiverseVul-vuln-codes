static int fts3DoIncrmerge(
  Fts3Table *p,                   /* FTS3 table handle */
  const char *zParam              /* Nul-terminated string containing "A,B" */
){
  int rc;
  int nMin = (FTS3_MERGE_COUNT / 2);
  int nMerge = 0;
  const char *z = zParam;

  /* Read the first integer value */
  nMerge = fts3Getint(&z);

  /* If the first integer value is followed by a ',',  read the second
  ** integer value. */
  if( z[0]==',' && z[1]!='\0' ){
    z++;
    nMin = fts3Getint(&z);
  }

  if( z[0]!='\0' || nMin<2 ){
    rc = SQLITE_ERROR;
  }else{
    rc = SQLITE_OK;
    if( !p->bHasStat ){
      assert( p->bFts4==0 );
      sqlite3Fts3CreateStatTable(&rc, p);
    }
    if( rc==SQLITE_OK ){
      rc = sqlite3Fts3Incrmerge(p, nMerge, nMin);
    }
    sqlite3Fts3SegmentsClose(p);
  }
  return rc;
}