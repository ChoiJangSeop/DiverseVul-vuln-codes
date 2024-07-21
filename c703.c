httpSpecialDoSide(HTTPRequestPtr requestor)
{
    HTTPConnectionPtr client = requestor->connection;

    if(client->reqlen - client->reqbegin >= client->bodylen) {
        AtomPtr data;
        data = internAtomN(client->reqbuf + client->reqbegin,
                           client->reqlen - client->reqbegin);
        client->reqbegin = 0;
        client->reqlen = 0;
        if(data == NULL) {
            do_log(L_ERROR, "Couldn't allocate data.\n");
            httpClientError(requestor, 500,
                            internAtom("Couldn't allocate data"));
            return 1;
        }
        httpSpecialDoSideFinish(data, requestor);
        return 1;
    }

    if(client->reqlen - client->reqbegin >= CHUNK_SIZE) {
        httpClientError(requestor, 500, internAtom("POST too large"));
        return 1;
    }

    if(client->reqbegin > 0 && client->reqlen > client->reqbegin) {
        memmove(client->reqbuf, client->reqbuf + client->reqbegin,
                client->reqlen - client->reqbegin);
    }
    client->reqlen -= client->reqbegin;
    client->reqbegin = 0;

    do_stream(IO_READ | IO_NOTNOW, client->fd,
              client->reqlen, client->reqbuf, CHUNK_SIZE,
              httpSpecialClientSideHandler, client);
    return 1;
}