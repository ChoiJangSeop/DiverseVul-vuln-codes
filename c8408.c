static int fts3SelectLeaf(
  Fts3Table *p,                   /* Virtual table handle */
  const char *zTerm,              /* Term to select leaves for */
  int nTerm,                      /* Size of term zTerm in bytes */
  const char *zNode,              /* Buffer containing segment interior node */
  int nNode,                      /* Size of buffer at zNode */
  sqlite3_int64 *piLeaf,          /* Selected leaf node */
  sqlite3_int64 *piLeaf2          /* Selected leaf node */
){
  int rc = SQLITE_OK;             /* Return code */
  int iHeight;                    /* Height of this node in tree */

  assert( piLeaf || piLeaf2 );

  fts3GetVarint32(zNode, &iHeight);
  rc = fts3ScanInteriorNode(zTerm, nTerm, zNode, nNode, piLeaf, piLeaf2);
  assert( !piLeaf2 || !piLeaf || rc!=SQLITE_OK || (*piLeaf<=*piLeaf2) );

  if( rc==SQLITE_OK && iHeight>1 ){
    char *zBlob = 0;              /* Blob read from %_segments table */
    int nBlob = 0;                /* Size of zBlob in bytes */

    if( piLeaf && piLeaf2 && (*piLeaf!=*piLeaf2) ){
      rc = sqlite3Fts3ReadBlock(p, *piLeaf, &zBlob, &nBlob, 0);
      if( rc==SQLITE_OK ){
        rc = fts3SelectLeaf(p, zTerm, nTerm, zBlob, nBlob, piLeaf, 0);
      }
      sqlite3_free(zBlob);
      piLeaf = 0;
      zBlob = 0;
    }

    if( rc==SQLITE_OK ){
      rc = sqlite3Fts3ReadBlock(p, piLeaf?*piLeaf:*piLeaf2, &zBlob, &nBlob, 0);
    }
    if( rc==SQLITE_OK ){
      int iNewHeight = 0;
      fts3GetVarint32(zBlob, &iNewHeight);
      if( iNewHeight<=iHeight ){
        rc = FTS_CORRUPT_VTAB;
      }else{
        rc = fts3SelectLeaf(p, zTerm, nTerm, zBlob, nBlob, piLeaf, piLeaf2);
      }
    }
    sqlite3_free(zBlob);
  }

  return rc;
}