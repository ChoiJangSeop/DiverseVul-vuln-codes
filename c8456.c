int sqlite3CheckObjectName(
  Parse *pParse,            /* Parsing context */
  const char *zName,        /* Name of the object to check */
  const char *zType,        /* Type of this object */
  const char *zTblName      /* Parent table name for triggers and indexes */
){
  sqlite3 *db = pParse->db;
  if( sqlite3WritableSchema(db) || db->init.imposterTable ){
    /* Skip these error checks for writable_schema=ON */
    return SQLITE_OK;
  }
  if( db->init.busy ){
    if( sqlite3_stricmp(zType, db->init.azInit[0])
     || sqlite3_stricmp(zName, db->init.azInit[1])
     || sqlite3_stricmp(zTblName, db->init.azInit[2])
    ){
      if( sqlite3Config.bExtraSchemaChecks ){
        sqlite3ErrorMsg(pParse, ""); /* corruptSchema() will supply the error */
        return SQLITE_ERROR;
      }
    }
  }else{
    if( pParse->nested==0 
     && 0==sqlite3StrNICmp(zName, "sqlite_", 7)
    ){
      sqlite3ErrorMsg(pParse, "object name reserved for internal use: %s",
                      zName);
      return SQLITE_ERROR;
    }
  }
  return SQLITE_OK;
}