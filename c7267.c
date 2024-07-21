int XMLRPC_AddValueToVector(XMLRPC_VALUE target, XMLRPC_VALUE source) {
   if(target && source) {
      if(target->type == xmlrpc_vector && target->v && 
         target->v->q && target->v->type != xmlrpc_vector_none) {

         /* guard against putting value of unknown type into vector */
         switch(source->type) {
            case xmlrpc_empty:
            case xmlrpc_base64:
            case xmlrpc_boolean:
            case xmlrpc_datetime:
            case xmlrpc_double:
            case xmlrpc_int:
            case xmlrpc_string:
            case xmlrpc_vector:
               /* Guard against putting a key/val pair into an array vector */
               if( !(source->id.len && target->v->type == xmlrpc_vector_array) ) {
					if (isDuplicateEntry (target, source)
						 || Q_PushTail (target->v->q, XMLRPC_CopyValue (source))) {
                     return 1;
                  }
               }
               else {
					fprintf (stderr,
								"xmlrpc: attempted to add key/val pair to vector of type array\n");
               }
               break;
            default:
				fprintf (stderr,
							"xmlrpc: attempted to add value of unknown type to vector\n");
               break;
         }
      }
   }
   return 0;
}