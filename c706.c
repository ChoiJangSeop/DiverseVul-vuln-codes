httpLocalRequest(ObjectPtr object, int method, int from, int to,
                 HTTPRequestPtr requestor, void *closure)
{
    if(object->requestor == NULL)
        object->requestor = requestor;

    if(!disableLocalInterface && urlIsSpecial(object->key, object->key_size))
        return httpSpecialRequest(object, method, from, to, 
                                  requestor, closure);

    if(method >= METHOD_POST) {
        httpClientError(requestor, 405, internAtom("Method not allowed"));
        requestor->connection->flags &= ~CONN_READER;
        return 1;
    }

    /* objectFillFromDisk already did the real work but we have to
       make sure we don't get into an infinite loop. */
    if(object->flags & OBJECT_INITIAL) {
        abortObject(object, 404, internAtom("Not found"));
    }
    object->age = current_time.tv_sec;
    object->date = current_time.tv_sec;

    object->flags &= ~OBJECT_VALIDATING;
    notifyObject(object);
    return 1;
}