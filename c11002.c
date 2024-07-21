static void DecodeTCPOptions(Packet *p, const uint8_t *pkt, uint16_t pktlen)
{
    uint8_t tcp_opt_cnt = 0;
    TCPOpt tcp_opts[TCP_OPTMAX];

    uint16_t plen = pktlen;
    while (plen)
    {
        const uint8_t type = *pkt;

        /* single byte options */
        if (type == TCP_OPT_EOL) {
            break;
        } else if (type == TCP_OPT_NOP) {
            pkt++;
            plen--;

        /* multibyte options */
        } else {
            if (plen < 2) {
                break;
            }

            const uint8_t olen = *(pkt+1);

            /* we already know that the total options len is valid,
             * so here the len of the specific option must be bad.
             * Also check for invalid lengths 0 and 1. */
            if (unlikely(olen > plen || olen < 2)) {
                ENGINE_SET_INVALID_EVENT(p, TCP_OPT_INVALID_LEN);
                return;
            }

            tcp_opts[tcp_opt_cnt].type = type;
            tcp_opts[tcp_opt_cnt].len  = olen;
            tcp_opts[tcp_opt_cnt].data = (olen > 2) ? (pkt+2) : NULL;

            /* we are parsing the most commonly used opts to prevent
             * us from having to walk the opts list for these all the
             * time. */
            switch (type) {
                case TCP_OPT_WS:
                    if (olen != TCP_OPT_WS_LEN) {
                        ENGINE_SET_EVENT(p,TCP_OPT_INVALID_LEN);
                    } else {
                        if (p->tcpvars.ws.type != 0) {
                            ENGINE_SET_EVENT(p,TCP_OPT_DUPLICATE);
                        } else {
                            SET_OPTS(p->tcpvars.ws, tcp_opts[tcp_opt_cnt]);
                        }
                    }
                    break;
                case TCP_OPT_MSS:
                    if (olen != TCP_OPT_MSS_LEN) {
                        ENGINE_SET_EVENT(p,TCP_OPT_INVALID_LEN);
                    } else {
                        if (p->tcpvars.mss.type != 0) {
                            ENGINE_SET_EVENT(p,TCP_OPT_DUPLICATE);
                        } else {
                            SET_OPTS(p->tcpvars.mss, tcp_opts[tcp_opt_cnt]);
                        }
                    }
                    break;
                case TCP_OPT_SACKOK:
                    if (olen != TCP_OPT_SACKOK_LEN) {
                        ENGINE_SET_EVENT(p,TCP_OPT_INVALID_LEN);
                    } else {
                        if (p->tcpvars.sackok.type != 0) {
                            ENGINE_SET_EVENT(p,TCP_OPT_DUPLICATE);
                        } else {
                            SET_OPTS(p->tcpvars.sackok, tcp_opts[tcp_opt_cnt]);
                        }
                    }
                    break;
                case TCP_OPT_TS:
                    if (olen != TCP_OPT_TS_LEN) {
                        ENGINE_SET_EVENT(p,TCP_OPT_INVALID_LEN);
                    } else {
                        if (p->tcpvars.ts_set) {
                            ENGINE_SET_EVENT(p,TCP_OPT_DUPLICATE);
                        } else {
                            uint32_t values[2];
                            memcpy(&values, tcp_opts[tcp_opt_cnt].data, sizeof(values));
                            p->tcpvars.ts_val = SCNtohl(values[0]);
                            p->tcpvars.ts_ecr = SCNtohl(values[1]);
                            p->tcpvars.ts_set = true;
                        }
                    }
                    break;
                case TCP_OPT_SACK:
                    SCLogDebug("SACK option, len %u", olen);
                    if ((olen != 2) &&
                           (olen < TCP_OPT_SACK_MIN_LEN ||
                            olen > TCP_OPT_SACK_MAX_LEN ||
                            !((olen - 2) % 8 == 0)))
                    {
                        ENGINE_SET_EVENT(p,TCP_OPT_INVALID_LEN);
                    } else {
                        if (p->tcpvars.sack.type != 0) {
                            ENGINE_SET_EVENT(p,TCP_OPT_DUPLICATE);
                        } else {
                            SET_OPTS(p->tcpvars.sack, tcp_opts[tcp_opt_cnt]);
                        }
                    }
                    break;
                case TCP_OPT_TFO:
                    SCLogDebug("TFO option, len %u", olen);
                    if ((olen != 2) && (olen < TCP_OPT_TFO_MIN_LEN || olen > TCP_OPT_TFO_MAX_LEN ||
                                               !(((olen - 2) & 0x1) == 0))) {
                        ENGINE_SET_EVENT(p,TCP_OPT_INVALID_LEN);
                    } else {
                        if (p->tcpvars.tfo.type != 0) {
                            ENGINE_SET_EVENT(p,TCP_OPT_DUPLICATE);
                        } else {
                            SET_OPTS(p->tcpvars.tfo, tcp_opts[tcp_opt_cnt]);
                        }
                    }
                    break;
                /* experimental options, could be TFO */
                case TCP_OPT_EXP1:
                case TCP_OPT_EXP2:
                    SCLogDebug("TCP EXP option, len %u", olen);
                    if (olen == 4 || olen == 12) {
                        uint16_t magic = SCNtohs(*(uint16_t *)tcp_opts[tcp_opt_cnt].data);
                        if (magic == 0xf989) {
                            if (p->tcpvars.tfo.type != 0) {
                                ENGINE_SET_EVENT(p,TCP_OPT_DUPLICATE);
                            } else {
                                SET_OPTS(p->tcpvars.tfo, tcp_opts[tcp_opt_cnt]);
                                p->tcpvars.tfo.type = TCP_OPT_TFO; // treat as regular TFO
                            }
                        }
                    } else {
                        ENGINE_SET_EVENT(p,TCP_OPT_INVALID_LEN);
                    }
                    break;
            }

            pkt += olen;
            plen -= olen;
            tcp_opt_cnt++;
        }
    }
}