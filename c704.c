httpSpecialClientSideHandler(int status,
                             FdEventHandlerPtr event,
                             StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;
    HTTPRequestPtr request = connection->request;
    int push;

    if((request->object->flags & OBJECT_ABORTED) || 
       !(request->object->flags & OBJECT_INPROGRESS)) {
        httpClientDiscardBody(connection);
        httpClientError(request, 503, internAtom("Post aborted"));
        return 1;
    }
        
    if(status < 0) {
        do_log_error(L_ERROR, -status, "Reading from client");
        if(status == -EDOGRACEFUL)
            httpClientFinish(connection, 1);
        else
            httpClientFinish(connection, 2);
        return 1;
    }

    push = MIN(srequest->offset - connection->reqlen,
               connection->bodylen - connection->reqoffset);
    if(push > 0) {
        connection->reqlen += push;
        httpSpecialDoSide(request);
    }

    do_log(L_ERROR, "Incomplete client request.\n");
    connection->flags &= ~CONN_READER;
    httpClientRawError(connection, 502,
                       internAtom("Incomplete client request"), 1);
    return 1;
}