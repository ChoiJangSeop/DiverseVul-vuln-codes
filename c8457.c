static int isShadowTableName(sqlite3 *db, char *zName){
  char *zTail;                  /* Pointer to the last "_" in zName */
  Table *pTab;                  /* Table that zName is a shadow of */
  Module *pMod;                 /* Module for the virtual table */

  zTail = strrchr(zName, '_');
  if( zTail==0 ) return 0;
  *zTail = 0;
  pTab = sqlite3FindTable(db, zName, 0);
  *zTail = '_';
  if( pTab==0 ) return 0;
  if( !IsVirtual(pTab) ) return 0;
  pMod = (Module*)sqlite3HashFind(&db->aModule, pTab->azModuleArg[0]);
  if( pMod==0 ) return 0;
  if( pMod->pModule->iVersion<3 ) return 0;
  if( pMod->pModule->xShadowName==0 ) return 0;
  return pMod->pModule->xShadowName(zTail+1);
}