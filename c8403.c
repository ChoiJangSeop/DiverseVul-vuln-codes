static int fts3DoAutoincrmerge(
  Fts3Table *p,                   /* FTS3 table handle */
  const char *zParam              /* Nul-terminated string containing boolean */
){
  int rc = SQLITE_OK;
  sqlite3_stmt *pStmt = 0;
  p->nAutoincrmerge = fts3Getint(&zParam);
  if( p->nAutoincrmerge==1 || p->nAutoincrmerge>FTS3_MERGE_COUNT ){
    p->nAutoincrmerge = 8;
  }
  if( !p->bHasStat ){
    assert( p->bFts4==0 );
    sqlite3Fts3CreateStatTable(&rc, p);
    if( rc ) return rc;
  }
  rc = fts3SqlStmt(p, SQL_REPLACE_STAT, &pStmt, 0);
  if( rc ) return rc;
  sqlite3_bind_int(pStmt, 1, FTS_STAT_AUTOINCRMERGE);
  sqlite3_bind_int(pStmt, 2, p->nAutoincrmerge);
  sqlite3_step(pStmt);
  rc = sqlite3_reset(pStmt);
  return rc;
}