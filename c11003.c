static int StreamTcpPacketStateClosed(ThreadVars *tv, Packet *p,
        StreamTcpThread *stt, TcpSession *ssn, PacketQueueNoLock *pq)
{
    if (ssn == NULL)
        return -1;

    if (p->tcph->th_flags & TH_RST) {
        SCLogDebug("RST on closed state");
        return 0;
    }

    TcpStream *stream = NULL, *ostream = NULL;
    if (PKT_IS_TOSERVER(p)) {
        stream = &ssn->client;
        ostream = &ssn->server;
    } else {
        stream = &ssn->server;
        ostream = &ssn->client;
    }

    SCLogDebug("stream %s ostream %s",
            stream->flags & STREAMTCP_STREAM_FLAG_RST_RECV?"true":"false",
            ostream->flags & STREAMTCP_STREAM_FLAG_RST_RECV ? "true":"false");

    /* if we've seen a RST on our direction, but not on the other
     * see if we perhaps need to continue processing anyway. */
    if ((stream->flags & STREAMTCP_STREAM_FLAG_RST_RECV) == 0) {
        if (ostream->flags & STREAMTCP_STREAM_FLAG_RST_RECV) {
            if (StreamTcpStateDispatch(tv, p, stt, ssn, &stt->pseudo_queue, ssn->pstate) < 0)
                return -1;
        }
    }
    return 0;
}