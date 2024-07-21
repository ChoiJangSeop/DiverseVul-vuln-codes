httpClientDiscardBody(HTTPConnectionPtr connection)
{
    TimeEventHandlerPtr handler;

    assert(connection->reqoffset == 0);
    assert(connection->flags & CONN_READER);

    if(connection->reqte != TE_IDENTITY)
        goto fail;

    if(connection->bodylen < 0)
        goto fail;

    if(connection->bodylen < connection->reqlen - connection->reqbegin) {
        connection->reqbegin += connection->bodylen;
        connection->bodylen = 0;
    } else {
        connection->bodylen -= connection->reqlen - connection->reqbegin;
        connection->reqbegin = 0;
        connection->reqlen = 0;
        httpConnectionDestroyReqbuf(connection);
    }
    connection->reqte = TE_UNKNOWN;

    if(connection->bodylen > 0) {
        httpSetTimeout(connection, clientTimeout);
        do_stream_buf(IO_READ | IO_NOTNOW,
                      connection->fd, connection->reqlen,
                      &connection->reqbuf, CHUNK_SIZE,
                      httpClientDiscardHandler, connection);
        return 1;
    }

    if(connection->reqlen > connection->reqbegin &&
       (connection->reqlen - connection->reqbegin) > 0) {
        memmove(connection->reqbuf, connection->reqbuf + connection->reqbegin,
                connection->reqlen - connection->reqbegin);
        connection->reqlen -= connection->reqbegin;
        connection->reqbegin = 0;
    } else {
        connection->reqlen = 0;
        connection->reqbegin = 0;
    }

    httpSetTimeout(connection, clientTimeout);
    /* We need to delay in order to make sure the previous request
       gets queued on the server side.  IO_NOTNOW isn't strong enough
       for that due to IO_IMMEDIATE. */
    handler = scheduleTimeEvent(-1, httpClientDelayed,
                                sizeof(connection), &connection);
    if(handler == NULL) {
        do_log(L_ERROR, "Couldn't schedule reading from client.");
        goto fail;
    }
    return 1;

 fail:
    connection->reqlen = 0;
    connection->reqbegin = 0;
    connection->bodylen = 0;
    connection->reqte = TE_UNKNOWN;
    shutdown(connection->fd, 2);
    handler = scheduleTimeEvent(-1, httpClientDelayed,
                                sizeof(connection), &connection);
    if(handler == NULL) {
        do_log(L_ERROR, "Couldn't schedule reading from client.");
        connection->flags &= ~CONN_READER;
    }
    return 1;
}