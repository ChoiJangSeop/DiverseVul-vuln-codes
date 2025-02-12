void simplestring_addn(simplestring* target, const char* source, int add_len) {
   if(target && source) {
      if(!target->str) {
         simplestring_init_str(target);
      }
      if(target->len + add_len + 1 > target->size) {
         /* newsize is current length + new length */
         int newsize = target->len + add_len + 1;
         int incr = target->size * 2;

         /* align to SIMPLESTRING_INCR increments */
         newsize = newsize - (newsize % incr) + incr;
         target->str = (char*)realloc(target->str, newsize);

         target->size = target->str ? newsize : 0;
      }

      if(target->str) {
         if(add_len) {
            memcpy(target->str + target->len, source, add_len);
         }
         target->len += add_len;
         target->str[target->len] = 0; /* null terminate */
      }
   }
}