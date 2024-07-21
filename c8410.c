static int fts3EvalPhraseStart(Fts3Cursor *pCsr, int bOptOk, Fts3Phrase *p){
  Fts3Table *pTab = (Fts3Table *)pCsr->base.pVtab;
  int rc = SQLITE_OK;             /* Error code */
  int i;

  /* Determine if doclists may be loaded from disk incrementally. This is
  ** possible if the bOptOk argument is true, the FTS doclists will be
  ** scanned in forward order, and the phrase consists of 
  ** MAX_INCR_PHRASE_TOKENS or fewer tokens, none of which are are "^first"
  ** tokens or prefix tokens that cannot use a prefix-index.  */
  int bHaveIncr = 0;
  int bIncrOk = (bOptOk 
   && pCsr->bDesc==pTab->bDescIdx 
   && p->nToken<=MAX_INCR_PHRASE_TOKENS && p->nToken>0
#ifdef SQLITE_TEST
   && pTab->bNoIncrDoclist==0
#endif
  );
  for(i=0; bIncrOk==1 && i<p->nToken; i++){
    Fts3PhraseToken *pToken = &p->aToken[i];
    if( pToken->bFirst || (pToken->pSegcsr!=0 && !pToken->pSegcsr->bLookup) ){
      bIncrOk = 0;
    }
    if( pToken->pSegcsr ) bHaveIncr = 1;
  }

  if( bIncrOk && bHaveIncr ){
    /* Use the incremental approach. */
    int iCol = (p->iColumn >= pTab->nColumn ? -1 : p->iColumn);
    for(i=0; rc==SQLITE_OK && i<p->nToken; i++){
      Fts3PhraseToken *pToken = &p->aToken[i];
      Fts3MultiSegReader *pSegcsr = pToken->pSegcsr;
      if( pSegcsr ){
        rc = sqlite3Fts3MsrIncrStart(pTab, pSegcsr, iCol, pToken->z, pToken->n);
      }
    }
    p->bIncr = 1;
  }else{
    /* Load the full doclist for the phrase into memory. */
    rc = fts3EvalPhraseLoad(pCsr, p);
    p->bIncr = 0;
  }

  assert( rc!=SQLITE_OK || p->nToken<1 || p->aToken[0].pSegcsr==0 || p->bIncr );
  return rc;
}