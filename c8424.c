static int renameColumnSelectCb(Walker *pWalker, Select *p){
  renameWalkWith(pWalker, p);
  return WRC_Continue;
}