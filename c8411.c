static int fts3NodeAddTerm(
  Fts3Table *p,                   /* Virtual table handle */
  SegmentNode **ppTree,           /* IN/OUT: SegmentNode handle */ 
  int isCopyTerm,                 /* True if zTerm/nTerm is transient */
  const char *zTerm,              /* Pointer to buffer containing term */
  int nTerm                       /* Size of term in bytes */
){
  SegmentNode *pTree = *ppTree;
  int rc;
  SegmentNode *pNew;

  /* First try to append the term to the current node. Return early if 
  ** this is possible.
  */
  if( pTree ){
    int nData = pTree->nData;     /* Current size of node in bytes */
    int nReq = nData;             /* Required space after adding zTerm */
    int nPrefix;                  /* Number of bytes of prefix compression */
    int nSuffix;                  /* Suffix length */

    nPrefix = fts3PrefixCompress(pTree->zTerm, pTree->nTerm, zTerm, nTerm);
    nSuffix = nTerm-nPrefix;

    nReq += sqlite3Fts3VarintLen(nPrefix)+sqlite3Fts3VarintLen(nSuffix)+nSuffix;
    if( nReq<=p->nNodeSize || !pTree->zTerm ){

      if( nReq>p->nNodeSize ){
        /* An unusual case: this is the first term to be added to the node
        ** and the static node buffer (p->nNodeSize bytes) is not large
        ** enough. Use a separately malloced buffer instead This wastes
        ** p->nNodeSize bytes, but since this scenario only comes about when
        ** the database contain two terms that share a prefix of almost 2KB, 
        ** this is not expected to be a serious problem. 
        */
        assert( pTree->aData==(char *)&pTree[1] );
        pTree->aData = (char *)sqlite3_malloc(nReq);
        if( !pTree->aData ){
          return SQLITE_NOMEM;
        }
      }

      if( pTree->zTerm ){
        /* There is no prefix-length field for first term in a node */
        nData += sqlite3Fts3PutVarint(&pTree->aData[nData], nPrefix);
      }

      nData += sqlite3Fts3PutVarint(&pTree->aData[nData], nSuffix);
      memcpy(&pTree->aData[nData], &zTerm[nPrefix], nSuffix);
      pTree->nData = nData + nSuffix;
      pTree->nEntry++;

      if( isCopyTerm ){
        if( pTree->nMalloc<nTerm ){
          char *zNew = sqlite3_realloc(pTree->zMalloc, nTerm*2);
          if( !zNew ){
            return SQLITE_NOMEM;
          }
          pTree->nMalloc = nTerm*2;
          pTree->zMalloc = zNew;
        }
        pTree->zTerm = pTree->zMalloc;
        memcpy(pTree->zTerm, zTerm, nTerm);
        pTree->nTerm = nTerm;
      }else{
        pTree->zTerm = (char *)zTerm;
        pTree->nTerm = nTerm;
      }
      return SQLITE_OK;
    }
  }

  /* If control flows to here, it was not possible to append zTerm to the
  ** current node. Create a new node (a right-sibling of the current node).
  ** If this is the first node in the tree, the term is added to it.
  **
  ** Otherwise, the term is not added to the new node, it is left empty for
  ** now. Instead, the term is inserted into the parent of pTree. If pTree 
  ** has no parent, one is created here.
  */
  pNew = (SegmentNode *)sqlite3_malloc(sizeof(SegmentNode) + p->nNodeSize);
  if( !pNew ){
    return SQLITE_NOMEM;
  }
  memset(pNew, 0, sizeof(SegmentNode));
  pNew->nData = 1 + FTS3_VARINT_MAX;
  pNew->aData = (char *)&pNew[1];

  if( pTree ){
    SegmentNode *pParent = pTree->pParent;
    rc = fts3NodeAddTerm(p, &pParent, isCopyTerm, zTerm, nTerm);
    if( pTree->pParent==0 ){
      pTree->pParent = pParent;
    }
    pTree->pRight = pNew;
    pNew->pLeftmost = pTree->pLeftmost;
    pNew->pParent = pParent;
    pNew->zMalloc = pTree->zMalloc;
    pNew->nMalloc = pTree->nMalloc;
    pTree->zMalloc = 0;
  }else{
    pNew->pLeftmost = pNew;
    rc = fts3NodeAddTerm(p, &pNew, isCopyTerm, zTerm, nTerm); 
  }

  *ppTree = pNew;
  return rc;
}