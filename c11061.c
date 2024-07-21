time_t base64totime_t(char* s, database* db, const char* field_name){
  
  byte* b=decode_base64(s,strlen(s),NULL);
  char* endp;
  
  if (b==NULL||strcmp(s,"0")==0) {
    
    /* Should we print error here? */
    free(b);
    
    return 0;
  } else {
    time_t t = strtol((char *)b,&endp,10);
    
    if (endp[0]!='\0') {
      LOG_DB_FORMAT_LINE(LOG_LEVEL_WARNING, could not read '%s' from database: strtoll failed for '%s' (base64 encoded value: '%s'), field_name, b, s)
      free(b);
      return 0;
    }
    log_msg(LOG_LEVEL_DEBUG, "base64totime_t: converted '%s': '%s' to %lld (base64 encoded value '%s')", field_name, b, (long long) t, s);
    free(b);
    return t;
  }
  
  
}