static int fts3SegReaderNext(
  Fts3Table *p, 
  Fts3SegReader *pReader,
  int bIncr
){
  int rc;                         /* Return code of various sub-routines */
  char *pNext;                    /* Cursor variable */
  int nPrefix;                    /* Number of bytes in term prefix */
  int nSuffix;                    /* Number of bytes in term suffix */

  if( !pReader->aDoclist ){
    pNext = pReader->aNode;
  }else{
    pNext = &pReader->aDoclist[pReader->nDoclist];
  }

  if( !pNext || pNext>=&pReader->aNode[pReader->nNode] ){

    if( fts3SegReaderIsPending(pReader) ){
      Fts3HashElem *pElem = *(pReader->ppNextElem);
      sqlite3_free(pReader->aNode);
      pReader->aNode = 0;
      if( pElem ){
        char *aCopy;
        PendingList *pList = (PendingList *)fts3HashData(pElem);
        int nCopy = pList->nData+1;
        pReader->zTerm = (char *)fts3HashKey(pElem);
        pReader->nTerm = fts3HashKeysize(pElem);
        aCopy = (char*)sqlite3_malloc(nCopy);
        if( !aCopy ) return SQLITE_NOMEM;
        memcpy(aCopy, pList->aData, nCopy);
        pReader->nNode = pReader->nDoclist = nCopy;
        pReader->aNode = pReader->aDoclist = aCopy;
        pReader->ppNextElem++;
        assert( pReader->aNode );
      }
      return SQLITE_OK;
    }

    fts3SegReaderSetEof(pReader);

    /* If iCurrentBlock>=iLeafEndBlock, this is an EOF condition. All leaf 
    ** blocks have already been traversed.  */
#ifdef CORRUPT_DB
    assert( pReader->iCurrentBlock<=pReader->iLeafEndBlock || CORRUPT_DB );
#endif
    if( pReader->iCurrentBlock>=pReader->iLeafEndBlock ){
      return SQLITE_OK;
    }

    rc = sqlite3Fts3ReadBlock(
        p, ++pReader->iCurrentBlock, &pReader->aNode, &pReader->nNode, 
        (bIncr ? &pReader->nPopulate : 0)
    );
    if( rc!=SQLITE_OK ) return rc;
    assert( pReader->pBlob==0 );
    if( bIncr && pReader->nPopulate<pReader->nNode ){
      pReader->pBlob = p->pSegments;
      p->pSegments = 0;
    }
    pNext = pReader->aNode;
  }

  assert( !fts3SegReaderIsPending(pReader) );

  rc = fts3SegReaderRequire(pReader, pNext, FTS3_VARINT_MAX*2);
  if( rc!=SQLITE_OK ) return rc;
  
  /* Because of the FTS3_NODE_PADDING bytes of padding, the following is 
  ** safe (no risk of overread) even if the node data is corrupted. */
  pNext += fts3GetVarint32(pNext, &nPrefix);
  pNext += fts3GetVarint32(pNext, &nSuffix);
  if( nSuffix<=0 
   || (&pReader->aNode[pReader->nNode] - pNext)<nSuffix
   || nPrefix>pReader->nTermAlloc
  ){
    return FTS_CORRUPT_VTAB;
  }

  /* Both nPrefix and nSuffix were read by fts3GetVarint32() and so are
  ** between 0 and 0x7FFFFFFF. But the sum of the two may cause integer
  ** overflow - hence the (i64) casts.  */
  if( (i64)nPrefix+nSuffix>(i64)pReader->nTermAlloc ){
    i64 nNew = ((i64)nPrefix+nSuffix)*2;
    char *zNew = sqlite3_realloc64(pReader->zTerm, nNew);
    if( !zNew ){
      return SQLITE_NOMEM;
    }
    pReader->zTerm = zNew;
    pReader->nTermAlloc = nNew;
  }

  rc = fts3SegReaderRequire(pReader, pNext, nSuffix+FTS3_VARINT_MAX);
  if( rc!=SQLITE_OK ) return rc;

  memcpy(&pReader->zTerm[nPrefix], pNext, nSuffix);
  pReader->nTerm = nPrefix+nSuffix;
  pNext += nSuffix;
  pNext += fts3GetVarint32(pNext, &pReader->nDoclist);
  pReader->aDoclist = pNext;
  pReader->pOffsetList = 0;

  /* Check that the doclist does not appear to extend past the end of the
  ** b-tree node. And that the final byte of the doclist is 0x00. If either 
  ** of these statements is untrue, then the data structure is corrupt.
  */
  if( pReader->nDoclist > pReader->nNode-(pReader->aDoclist-pReader->aNode)
   || (pReader->nPopulate==0 && pReader->aDoclist[pReader->nDoclist-1])
  ){
    return FTS_CORRUPT_VTAB;
  }
  return SQLITE_OK;
}