void XMLRPC_SetValueDateTime(XMLRPC_VALUE value, time_t time) {
   if(value) {
      char timeBuf[30];
      value->type = xmlrpc_datetime;
      value->i = time;

      timeBuf[0] = 0;

      date_to_ISO8601(time, timeBuf, sizeof(timeBuf));

      if(timeBuf[0]) {
         simplestring_clear(&value->str);
         simplestring_add(&value->str, timeBuf);
      }
   }
}