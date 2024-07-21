static int StreamTcpTest10 (void)
{
    Packet *p = SCMalloc(SIZE_OF_PACKET);
    FAIL_IF(unlikely(p == NULL));
    Flow f;
    ThreadVars tv;
    StreamTcpThread stt;
    TCPHdr tcph;
    uint8_t payload[4];
    memset(p, 0, SIZE_OF_PACKET);
    PacketQueue pq;
    memset(&pq,0,sizeof(PacketQueue));
    memset (&f, 0, sizeof(Flow));
    memset(&tv, 0, sizeof (ThreadVars));
    memset(&stt, 0, sizeof (StreamTcpThread));
    memset(&tcph, 0, sizeof (TCPHdr));
    FLOW_INITIALIZE(&f);
    p->flow = &f;

    StreamTcpUTInit(&stt.ra_ctx);
    stream_config.async_oneside = TRUE;

    tcph.th_win = htons(5480);
    tcph.th_seq = htonl(10);
    tcph.th_ack = htonl(11);
    tcph.th_flags = TH_SYN;
    p->tcph = &tcph;

    FAIL_IF(StreamTcpPacket(&tv, p, &stt, &pq) == -1);

    p->tcph->th_seq = htonl(11);
    p->tcph->th_ack = htonl(11);
    p->tcph->th_flags = TH_ACK;
    p->flowflags = FLOW_PKT_TOSERVER;

    FAIL_IF(StreamTcpPacket(&tv, p, &stt, &pq) == -1);

    p->tcph->th_seq = htonl(11);
    p->tcph->th_ack = htonl(11);
    p->tcph->th_flags = TH_ACK|TH_PUSH;
    p->flowflags = FLOW_PKT_TOSERVER;

    StreamTcpCreateTestPacket(payload, 0x42, 3, 4); /*BBB*/
    p->payload = payload;
    p->payload_len = 3;

    FAIL_IF(StreamTcpPacket(&tv, p, &stt, &pq) == -1);

    p->tcph->th_seq = htonl(6);
    p->tcph->th_ack = htonl(11);
    p->tcph->th_flags = TH_ACK|TH_PUSH;
    p->flowflags = FLOW_PKT_TOSERVER;

    StreamTcpCreateTestPacket(payload, 0x42, 3, 4); /*BBB*/
    p->payload = payload;
    p->payload_len = 3;

    FAIL_IF(StreamTcpPacket(&tv, p, &stt, &pq) == -1);

    FAIL_IF(((TcpSession *)(p->flow->protoctx))->state != TCP_ESTABLISHED);

    FAIL_IF(! (((TcpSession *)(p->flow->protoctx))->flags & STREAMTCP_FLAG_ASYNC));

    FAIL_IF(((TcpSession *)(p->flow->protoctx))->client.last_ack != 6 &&
            ((TcpSession *)(p->flow->protoctx))->server.next_seq != 11);

    StreamTcpSessionClear(p->flow->protoctx);

    SCFree(p);
    FLOW_DESTROY(&f);
    StreamTcpUTDeinit(stt.ra_ctx);
    PASS;
}