static int renameTableSelectCb(Walker *pWalker, Select *pSelect){
  int i;
  RenameCtx *p = pWalker->u.pRename;
  SrcList *pSrc = pSelect->pSrc;
  if( pSrc==0 ){
    assert( pWalker->pParse->db->mallocFailed );
    return WRC_Abort;
  }
  for(i=0; i<pSrc->nSrc; i++){
    struct SrcList_item *pItem = &pSrc->a[i];
    if( pItem->pTab==p->pTab ){
      renameTokenFind(pWalker->pParse, p, pItem->zName);
    }
  }
  renameWalkWith(pWalker, pSelect);

  return WRC_Continue;
}