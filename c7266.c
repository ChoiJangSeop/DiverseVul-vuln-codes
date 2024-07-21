void XMLRPC_CleanupValue(XMLRPC_VALUE value) {
   if(value) {
      if(value->iRefCount > 0) {
         value->iRefCount --;
      }

#ifdef XMLRPC_DEBUG_REFCOUNT
      if(value->id.str) {
			printf ("decremented refcount of %s, now %i\n", value->id.str,
					  value->iRefCount);
      }
      else {
			printf ("decremented refcount of 0x%x, now %i\n", value,
					  value->iRefCount);
      }
#endif

      if(value->type == xmlrpc_vector) {
         if(value->v) {
            if(value->iRefCount == 0) {
               XMLRPC_VALUE cur = (XMLRPC_VALUE)Q_Head(value->v->q);
               while( cur ) {
                  XMLRPC_CleanupValue(cur);
   
                  /* Make sure some idiot didn't include a vector as a child of itself
                   * and thus it would have already free'd these.
                   */
                  if(value->v && value->v->q) {
                     cur = Q_Next(value->v->q);
                  }
                  else {
                     break;
                  }
               }

               Q_Destroy(value->v->q);
               my_free(value->v->q);
               my_free(value->v);
            }
         }
      }


      if(value->iRefCount == 0) {

         /* guard against freeing invalid types */
         switch(value->type) {
            case xmlrpc_empty:
            case xmlrpc_base64:
            case xmlrpc_boolean:
            case xmlrpc_datetime:
            case xmlrpc_double:
            case xmlrpc_int:
            case xmlrpc_string:
            case xmlrpc_vector:
#ifdef XMLRPC_DEBUG_REFCOUNT
               if(value->id.str) {
                  printf("free'd %s\n", value->id.str);
               }
               else {
                  printf("free'd 0x%x\n", value);
               }
#endif 
               simplestring_free(&value->id);
               simplestring_free(&value->str);

               memset(value, 0, sizeof(STRUCT_XMLRPC_VALUE));
               my_free(value);
               break;
            default:
				fprintf (stderr,
							"xmlrpc: attempted to free value of invalid type\n");
               break;
         }
      }
   }
}