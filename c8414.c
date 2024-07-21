static int fts3MsrBufferData(
  Fts3MultiSegReader *pMsr,       /* Multi-segment-reader handle */
  char *pList,
  int nList
){
  if( nList>pMsr->nBuffer ){
    char *pNew;
    pMsr->nBuffer = nList*2;
    pNew = (char *)sqlite3_realloc(pMsr->aBuffer, pMsr->nBuffer);
    if( !pNew ) return SQLITE_NOMEM;
    pMsr->aBuffer = pNew;
  }

  memcpy(pMsr->aBuffer, pList, nList);
  return SQLITE_OK;
}