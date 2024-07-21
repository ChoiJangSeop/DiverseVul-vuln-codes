static int fts3SpecialInsert(Fts3Table *p, sqlite3_value *pVal){
  int rc = SQLITE_ERROR;           /* Return Code */
  const char *zVal = (const char *)sqlite3_value_text(pVal);
  int nVal = sqlite3_value_bytes(pVal);

  if( !zVal ){
    return SQLITE_NOMEM;
  }else if( nVal==8 && 0==sqlite3_strnicmp(zVal, "optimize", 8) ){
    rc = fts3DoOptimize(p, 0);
  }else if( nVal==7 && 0==sqlite3_strnicmp(zVal, "rebuild", 7) ){
    rc = fts3DoRebuild(p);
  }else if( nVal==15 && 0==sqlite3_strnicmp(zVal, "integrity-check", 15) ){
    rc = fts3DoIntegrityCheck(p);
  }else if( nVal>6 && 0==sqlite3_strnicmp(zVal, "merge=", 6) ){
    rc = fts3DoIncrmerge(p, &zVal[6]);
  }else if( nVal>10 && 0==sqlite3_strnicmp(zVal, "automerge=", 10) ){
    rc = fts3DoAutoincrmerge(p, &zVal[10]);
#if defined(SQLITE_DEBUG) || defined(SQLITE_TEST)
  }else{
    int v;
    if( nVal>9 && 0==sqlite3_strnicmp(zVal, "nodesize=", 9) ){
      v = atoi(&zVal[9]);
      if( v>=24 && v<=p->nPgsz-35 ) p->nNodeSize = v;
      rc = SQLITE_OK;
    }else if( nVal>11 && 0==sqlite3_strnicmp(zVal, "maxpending=", 9) ){
      v = atoi(&zVal[11]);
      if( v>=64 && v<=FTS3_MAX_PENDING_DATA ) p->nMaxPendingData = v;
      rc = SQLITE_OK;
    }else if( nVal>21 && 0==sqlite3_strnicmp(zVal,"test-no-incr-doclist=",21) ){
      p->bNoIncrDoclist = atoi(&zVal[21]);
      rc = SQLITE_OK;
    }
#endif
  }
  return rc;
}