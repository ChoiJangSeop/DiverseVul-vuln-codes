httpSpecialSideRequest(ObjectPtr object, int method, int from, int to,
                       HTTPRequestPtr requestor, void *closure)
{
    HTTPConnectionPtr client = requestor->connection;

    assert(client->request == requestor);

    if(method != METHOD_POST) {
        httpClientError(requestor, 405, internAtom("Method not allowed"));
        requestor->connection->flags &= ~CONN_READER;
        return 1;
    }

    return httpSpecialDoSide(requestor);
}