static int renameUnmapSelectCb(Walker *pWalker, Select *p){
  Parse *pParse = pWalker->pParse;
  int i;
  if( pParse->nErr ) return WRC_Abort;
  if( ALWAYS(p->pEList) ){
    ExprList *pList = p->pEList;
    for(i=0; i<pList->nExpr; i++){
      if( pList->a[i].zName ){
        sqlite3RenameTokenRemap(pParse, 0, (void*)pList->a[i].zName);
      }
    }
  }
  if( ALWAYS(p->pSrc) ){  /* Every Select as a SrcList, even if it is empty */
    SrcList *pSrc = p->pSrc;
    for(i=0; i<pSrc->nSrc; i++){
      sqlite3RenameTokenRemap(pParse, 0, (void*)pSrc->a[i].zName);
    }
  }

  renameWalkWith(pWalker, p);
  return WRC_Continue;
}