static int fts3AllocateSegdirIdx(
  Fts3Table *p, 
  int iLangid,                    /* Language id */
  int iIndex,                     /* Index for p->aIndex */
  int iLevel, 
  int *piIdx
){
  int rc;                         /* Return Code */
  sqlite3_stmt *pNextIdx;         /* Query for next idx at level iLevel */
  int iNext = 0;                  /* Result of query pNextIdx */

  assert( iLangid>=0 );
  assert( p->nIndex>=1 );

  /* Set variable iNext to the next available segdir index at level iLevel. */
  rc = fts3SqlStmt(p, SQL_NEXT_SEGMENT_INDEX, &pNextIdx, 0);
  if( rc==SQLITE_OK ){
    sqlite3_bind_int64(
        pNextIdx, 1, getAbsoluteLevel(p, iLangid, iIndex, iLevel)
    );
    if( SQLITE_ROW==sqlite3_step(pNextIdx) ){
      iNext = sqlite3_column_int(pNextIdx, 0);
    }
    rc = sqlite3_reset(pNextIdx);
  }

  if( rc==SQLITE_OK ){
    /* If iNext is FTS3_MERGE_COUNT, indicating that level iLevel is already
    ** full, merge all segments in level iLevel into a single iLevel+1
    ** segment and allocate (newly freed) index 0 at level iLevel. Otherwise,
    ** if iNext is less than FTS3_MERGE_COUNT, allocate index iNext.
    */
    if( iNext>=FTS3_MERGE_COUNT ){
      fts3LogMerge(16, getAbsoluteLevel(p, iLangid, iIndex, iLevel));
      rc = fts3SegmentMerge(p, iLangid, iIndex, iLevel);
      *piIdx = 0;
    }else{
      *piIdx = iNext;
    }
  }

  return rc;
}