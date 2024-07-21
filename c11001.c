static int StreamTcpValidateRst(TcpSession *ssn, Packet *p)
{

    uint8_t os_policy;

    if (ssn->flags & STREAMTCP_FLAG_TIMESTAMP) {
        if (!StreamTcpValidateTimestamp(ssn, p)) {
            SCReturnInt(0);
        }
    }

    /* Set up the os_policy to be used in validating the RST packets based on
       target system */
    if (PKT_IS_TOSERVER(p)) {
        if (ssn->server.os_policy == 0)
            StreamTcpSetOSPolicy(&ssn->server, p);

        os_policy = ssn->server.os_policy;

        if (p->tcph->th_flags & TH_ACK &&
                TCP_GET_ACK(p) && StreamTcpValidateAck(ssn, &ssn->server, p) == -1) {
            SCLogDebug("ssn %p: rejecting because of invalid ack value", ssn);
            StreamTcpSetEvent(p, STREAM_RST_INVALID_ACK);
            SCReturnInt(0);
        }

    } else {
        if (ssn->client.os_policy == 0)
            StreamTcpSetOSPolicy(&ssn->client, p);

        os_policy = ssn->client.os_policy;

        if (p->tcph->th_flags & TH_ACK &&
                TCP_GET_ACK(p) && StreamTcpValidateAck(ssn, &ssn->client, p) == -1) {
            SCLogDebug("ssn %p: rejecting because of invalid ack value", ssn);
            StreamTcpSetEvent(p, STREAM_RST_INVALID_ACK);
            SCReturnInt(0);
        }
    }

    if (ssn->flags & STREAMTCP_FLAG_ASYNC) {
        if (PKT_IS_TOSERVER(p)) {
            if (SEQ_GEQ(TCP_GET_SEQ(p), ssn->client.next_seq)) {
                SCLogDebug("ssn %p: ASYNC accept RST", ssn);
                return 1;
            }
        } else {
            if (SEQ_GEQ(TCP_GET_SEQ(p), ssn->server.next_seq)) {
                SCLogDebug("ssn %p: ASYNC accept RST", ssn);
                return 1;
            }
        }
        SCLogDebug("ssn %p: ASYNC reject RST", ssn);
        return 0;
    }

    switch (os_policy) {
        case OS_POLICY_HPUX11:
            if(PKT_IS_TOSERVER(p)){
                if(SEQ_GEQ(TCP_GET_SEQ(p), ssn->client.next_seq)) {
                    SCLogDebug("reset is Valid! Packet SEQ: %" PRIu32 "",
                                TCP_GET_SEQ(p));
                    return 1;
                } else {
                    SCLogDebug("reset is not Valid! Packet SEQ: %" PRIu32 " "
                               "and server SEQ: %" PRIu32 "", TCP_GET_SEQ(p),
                                ssn->client.next_seq);
                    return 0;
                }
            } else { /* implied to client */
                if(SEQ_GEQ(TCP_GET_SEQ(p), ssn->server.next_seq)) {
                    SCLogDebug("reset is valid! Packet SEQ: %" PRIu32 "",
                                TCP_GET_SEQ(p));
                    return 1;
                } else {
                    SCLogDebug("reset is not valid! Packet SEQ: %" PRIu32 " "
                               "and client SEQ: %" PRIu32 "", TCP_GET_SEQ(p),
                                ssn->server.next_seq);
                    return 0;
                }
            }
            break;
        case OS_POLICY_OLD_LINUX:
        case OS_POLICY_LINUX:
        case OS_POLICY_SOLARIS:
            if(PKT_IS_TOSERVER(p)){
                if(SEQ_GEQ((TCP_GET_SEQ(p)+p->payload_len),
                            ssn->client.last_ack))
                { /*window base is needed !!*/
                    if(SEQ_LT(TCP_GET_SEQ(p),
                              (ssn->client.next_seq + ssn->client.window)))
                    {
                        SCLogDebug("reset is Valid! Packet SEQ: %" PRIu32 "",
                                    TCP_GET_SEQ(p));
                        return 1;
                    }
                } else {
                    SCLogDebug("reset is not valid! Packet SEQ: %" PRIu32 " and"
                               " server SEQ: %" PRIu32 "", TCP_GET_SEQ(p),
                                ssn->client.next_seq);
                    return 0;
                }
            } else { /* implied to client */
                if(SEQ_GEQ((TCP_GET_SEQ(p) + p->payload_len),
                            ssn->server.last_ack))
                { /*window base is needed !!*/
                    if(SEQ_LT(TCP_GET_SEQ(p),
                                (ssn->server.next_seq + ssn->server.window)))
                    {
                        SCLogDebug("reset is Valid! Packet SEQ: %" PRIu32 "",
                                    TCP_GET_SEQ(p));
                        return 1;
                    }
                } else {
                    SCLogDebug("reset is not valid! Packet SEQ: %" PRIu32 " and"
                               " client SEQ: %" PRIu32 "", TCP_GET_SEQ(p),
                                 ssn->server.next_seq);
                    return 0;
                }
            }
            break;
        default:
        case OS_POLICY_BSD:
        case OS_POLICY_FIRST:
        case OS_POLICY_HPUX10:
        case OS_POLICY_IRIX:
        case OS_POLICY_MACOS:
        case OS_POLICY_LAST:
        case OS_POLICY_WINDOWS:
        case OS_POLICY_WINDOWS2K3:
        case OS_POLICY_VISTA:
            if(PKT_IS_TOSERVER(p)) {
                if(SEQ_EQ(TCP_GET_SEQ(p), ssn->client.next_seq)) {
                    SCLogDebug("reset is valid! Packet SEQ: %" PRIu32 "",
                               TCP_GET_SEQ(p));
                    return 1;
                } else {
                    SCLogDebug("reset is not valid! Packet SEQ: %" PRIu32 " "
                               "and server SEQ: %" PRIu32 "", TCP_GET_SEQ(p),
                               ssn->client.next_seq);
                    return 0;
                }
            } else { /* implied to client */
                if (SEQ_EQ(TCP_GET_SEQ(p), ssn->server.next_seq)) {
                    SCLogDebug("reset is valid! Packet SEQ: %" PRIu32 " Stream %u",
                                TCP_GET_SEQ(p), ssn->server.next_seq);
                    return 1;
                } else {
                    SCLogDebug("reset is not valid! Packet SEQ: %" PRIu32 " and"
                               " client SEQ: %" PRIu32 "",
                               TCP_GET_SEQ(p), ssn->server.next_seq);
                    return 0;
                }
            }
            break;
    }
    return 0;
}