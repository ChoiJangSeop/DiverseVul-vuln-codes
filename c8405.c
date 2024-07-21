static int fts3IncrmergeLoad(
  Fts3Table *p,                   /* Fts3 table handle */
  sqlite3_int64 iAbsLevel,        /* Absolute level of input segments */
  int iIdx,                       /* Index of candidate output segment */
  const char *zKey,               /* First key to write */
  int nKey,                       /* Number of bytes in nKey */
  IncrmergeWriter *pWriter        /* Populate this object */
){
  int rc;                         /* Return code */
  sqlite3_stmt *pSelect = 0;      /* SELECT to read %_segdir entry */

  rc = fts3SqlStmt(p, SQL_SELECT_SEGDIR, &pSelect, 0);
  if( rc==SQLITE_OK ){
    sqlite3_int64 iStart = 0;     /* Value of %_segdir.start_block */
    sqlite3_int64 iLeafEnd = 0;   /* Value of %_segdir.leaves_end_block */
    sqlite3_int64 iEnd = 0;       /* Value of %_segdir.end_block */
    const char *aRoot = 0;        /* Pointer to %_segdir.root buffer */
    int nRoot = 0;                /* Size of aRoot[] in bytes */
    int rc2;                      /* Return code from sqlite3_reset() */
    int bAppendable = 0;          /* Set to true if segment is appendable */

    /* Read the %_segdir entry for index iIdx absolute level (iAbsLevel+1) */
    sqlite3_bind_int64(pSelect, 1, iAbsLevel+1);
    sqlite3_bind_int(pSelect, 2, iIdx);
    if( sqlite3_step(pSelect)==SQLITE_ROW ){
      iStart = sqlite3_column_int64(pSelect, 1);
      iLeafEnd = sqlite3_column_int64(pSelect, 2);
      fts3ReadEndBlockField(pSelect, 3, &iEnd, &pWriter->nLeafData);
      if( pWriter->nLeafData<0 ){
        pWriter->nLeafData = pWriter->nLeafData * -1;
      }
      pWriter->bNoLeafData = (pWriter->nLeafData==0);
      nRoot = sqlite3_column_bytes(pSelect, 4);
      aRoot = sqlite3_column_blob(pSelect, 4);
    }else{
      return sqlite3_reset(pSelect);
    }

    /* Check for the zero-length marker in the %_segments table */
    rc = fts3IsAppendable(p, iEnd, &bAppendable);

    /* Check that zKey/nKey is larger than the largest key the candidate */
    if( rc==SQLITE_OK && bAppendable ){
      char *aLeaf = 0;
      int nLeaf = 0;

      rc = sqlite3Fts3ReadBlock(p, iLeafEnd, &aLeaf, &nLeaf, 0);
      if( rc==SQLITE_OK ){
        NodeReader reader;
        for(rc = nodeReaderInit(&reader, aLeaf, nLeaf);
            rc==SQLITE_OK && reader.aNode;
            rc = nodeReaderNext(&reader)
        ){
          assert( reader.aNode );
        }
        if( fts3TermCmp(zKey, nKey, reader.term.a, reader.term.n)<=0 ){
          bAppendable = 0;
        }
        nodeReaderRelease(&reader);
      }
      sqlite3_free(aLeaf);
    }

    if( rc==SQLITE_OK && bAppendable ){
      /* It is possible to append to this segment. Set up the IncrmergeWriter
      ** object to do so.  */
      int i;
      int nHeight = (int)aRoot[0];
      NodeWriter *pNode;

      pWriter->nLeafEst = (int)((iEnd - iStart) + 1)/FTS_MAX_APPENDABLE_HEIGHT;
      pWriter->iStart = iStart;
      pWriter->iEnd = iEnd;
      pWriter->iAbsLevel = iAbsLevel;
      pWriter->iIdx = iIdx;

      for(i=nHeight+1; i<FTS_MAX_APPENDABLE_HEIGHT; i++){
        pWriter->aNodeWriter[i].iBlock = pWriter->iStart + i*pWriter->nLeafEst;
      }

      pNode = &pWriter->aNodeWriter[nHeight];
      pNode->iBlock = pWriter->iStart + pWriter->nLeafEst*nHeight;
      blobGrowBuffer(&pNode->block, 
          MAX(nRoot, p->nNodeSize)+FTS3_NODE_PADDING, &rc
      );
      if( rc==SQLITE_OK ){
        memcpy(pNode->block.a, aRoot, nRoot);
        pNode->block.n = nRoot;
        memset(&pNode->block.a[nRoot], 0, FTS3_NODE_PADDING);
      }

      for(i=nHeight; i>=0 && rc==SQLITE_OK; i--){
        NodeReader reader;
        pNode = &pWriter->aNodeWriter[i];

        if( pNode->block.a){
          rc = nodeReaderInit(&reader, pNode->block.a, pNode->block.n);
          while( reader.aNode && rc==SQLITE_OK ) rc = nodeReaderNext(&reader);
          blobGrowBuffer(&pNode->key, reader.term.n, &rc);
          if( rc==SQLITE_OK ){
            memcpy(pNode->key.a, reader.term.a, reader.term.n);
            pNode->key.n = reader.term.n;
            if( i>0 ){
              char *aBlock = 0;
              int nBlock = 0;
              pNode = &pWriter->aNodeWriter[i-1];
              pNode->iBlock = reader.iChild;
              rc = sqlite3Fts3ReadBlock(p, reader.iChild, &aBlock, &nBlock, 0);
              blobGrowBuffer(&pNode->block, 
                  MAX(nBlock, p->nNodeSize)+FTS3_NODE_PADDING, &rc
              );
              if( rc==SQLITE_OK ){
                memcpy(pNode->block.a, aBlock, nBlock);
                pNode->block.n = nBlock;
                memset(&pNode->block.a[nBlock], 0, FTS3_NODE_PADDING);
              }
              sqlite3_free(aBlock);
            }
          }
        }
        nodeReaderRelease(&reader);
      }
    }

    rc2 = sqlite3_reset(pSelect);
    if( rc==SQLITE_OK ) rc = rc2;
  }

  return rc;
}