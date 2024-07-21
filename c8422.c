static void renameTableFunc(
  sqlite3_context *context,
  int NotUsed,
  sqlite3_value **argv
){
  sqlite3 *db = sqlite3_context_db_handle(context);
  const char *zDb = (const char*)sqlite3_value_text(argv[0]);
  const char *zInput = (const char*)sqlite3_value_text(argv[3]);
  const char *zOld = (const char*)sqlite3_value_text(argv[4]);
  const char *zNew = (const char*)sqlite3_value_text(argv[5]);
  int bTemp = sqlite3_value_int(argv[6]);
  UNUSED_PARAMETER(NotUsed);

  if( zInput && zOld && zNew ){
    Parse sParse;
    int rc;
    int bQuote = 1;
    RenameCtx sCtx;
    Walker sWalker;

#ifndef SQLITE_OMIT_AUTHORIZATION
    sqlite3_xauth xAuth = db->xAuth;
    db->xAuth = 0;
#endif

    sqlite3BtreeEnterAll(db);

    memset(&sCtx, 0, sizeof(RenameCtx));
    sCtx.pTab = sqlite3FindTable(db, zOld, zDb);
    memset(&sWalker, 0, sizeof(Walker));
    sWalker.pParse = &sParse;
    sWalker.xExprCallback = renameTableExprCb;
    sWalker.xSelectCallback = renameTableSelectCb;
    sWalker.u.pRename = &sCtx;

    rc = renameParseSql(&sParse, zDb, 1, db, zInput, bTemp);

    if( rc==SQLITE_OK ){
      int isLegacy = (db->flags & SQLITE_LegacyAlter);
      if( sParse.pNewTable ){
        Table *pTab = sParse.pNewTable;

        if( pTab->pSelect ){
          if( isLegacy==0 ){
            NameContext sNC;
            memset(&sNC, 0, sizeof(sNC));
            sNC.pParse = &sParse;

            sqlite3SelectPrep(&sParse, pTab->pSelect, &sNC);
            if( sParse.nErr ) rc = sParse.rc;
            sqlite3WalkSelect(&sWalker, pTab->pSelect);
          }
        }else{
          /* Modify any FK definitions to point to the new table. */
#ifndef SQLITE_OMIT_FOREIGN_KEY
          if( isLegacy==0 || (db->flags & SQLITE_ForeignKeys) ){
            FKey *pFKey;
            for(pFKey=pTab->pFKey; pFKey; pFKey=pFKey->pNextFrom){
              if( sqlite3_stricmp(pFKey->zTo, zOld)==0 ){
                renameTokenFind(&sParse, &sCtx, (void*)pFKey->zTo);
              }
            }
          }
#endif

          /* If this is the table being altered, fix any table refs in CHECK
          ** expressions. Also update the name that appears right after the
          ** "CREATE [VIRTUAL] TABLE" bit. */
          if( sqlite3_stricmp(zOld, pTab->zName)==0 ){
            sCtx.pTab = pTab;
            if( isLegacy==0 ){
              sqlite3WalkExprList(&sWalker, pTab->pCheck);
            }
            renameTokenFind(&sParse, &sCtx, pTab->zName);
          }
        }
      }

      else if( sParse.pNewIndex ){
        renameTokenFind(&sParse, &sCtx, sParse.pNewIndex->zName);
        if( isLegacy==0 ){
          sqlite3WalkExpr(&sWalker, sParse.pNewIndex->pPartIdxWhere);
        }
      }

#ifndef SQLITE_OMIT_TRIGGER
      else{
        Trigger *pTrigger = sParse.pNewTrigger;
        TriggerStep *pStep;
        if( 0==sqlite3_stricmp(sParse.pNewTrigger->table, zOld) 
            && sCtx.pTab->pSchema==pTrigger->pTabSchema
          ){
          renameTokenFind(&sParse, &sCtx, sParse.pNewTrigger->table);
        }

        if( isLegacy==0 ){
          rc = renameResolveTrigger(&sParse, bTemp ? 0 : zDb);
          if( rc==SQLITE_OK ){
            renameWalkTrigger(&sWalker, pTrigger);
            for(pStep=pTrigger->step_list; pStep; pStep=pStep->pNext){
              if( pStep->zTarget && 0==sqlite3_stricmp(pStep->zTarget, zOld) ){
                renameTokenFind(&sParse, &sCtx, pStep->zTarget);
              }
            }
          }
        }
      }
#endif
    }

    if( rc==SQLITE_OK ){
      rc = renameEditSql(context, &sCtx, zInput, zNew, bQuote);
    }
    if( rc!=SQLITE_OK ){
      if( sParse.zErrMsg ){
        renameColumnParseError(context, 0, argv[1], argv[2], &sParse);
      }else{
        sqlite3_result_error_code(context, rc);
      }
    }

    renameParseCleanup(&sParse);
    renameTokenFree(db, sCtx.pList);
    sqlite3BtreeLeaveAll(db);
#ifndef SQLITE_OMIT_AUTHORIZATION
    db->xAuth = xAuth;
#endif
  }

  return;
}