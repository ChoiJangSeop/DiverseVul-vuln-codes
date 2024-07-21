parse_packet(const unsigned char *from, struct interface *ifp,
             const unsigned char *packet, int packetlen)
{
    int i;
    const unsigned char *message;
    unsigned char type, len;
    int bodylen;
    struct neighbour *neigh;
    int have_router_id = 0, have_v4_prefix = 0, have_v6_prefix = 0,
        have_v4_nh = 0, have_v6_nh = 0;
    unsigned char router_id[8], v4_prefix[16], v6_prefix[16],
        v4_nh[16], v6_nh[16];
    int have_hello_rtt = 0;
    /* Content of the RTT sub-TLV on IHU messages. */
    unsigned int hello_send_us = 0, hello_rtt_receive_time = 0;
    babel_interface_nfo *babel_ifp = babel_get_if_nfo(ifp);

    if(babel_ifp->flags & BABEL_IF_TIMESTAMPS) {
        /* We want to track exactly when we received this packet. */
        gettime(&babel_now);
    }

    if(!linklocal(from)) {
        flog_err(EC_BABEL_PACKET,
		  "Received packet from non-local address %s.",
                  format_address(from));
        return;
    }

    if (babel_packet_examin (packet, packetlen)) {
        flog_err(EC_BABEL_PACKET,
		  "Received malformed packet on %s from %s.",
                  ifp->name, format_address(from));
        return;
    }

    neigh = find_neighbour(from, ifp);
    if(neigh == NULL) {
        flog_err(EC_BABEL_PACKET, "Couldn't allocate neighbour.");
        return;
    }

    DO_NTOHS(bodylen, packet + 2);

    if(bodylen + 4 > packetlen) {
        flog_err(EC_BABEL_PACKET, "Received truncated packet (%d + 4 > %d).",
                 bodylen, packetlen);
        bodylen = packetlen - 4;
    }

    i = 0;
    while(i < bodylen) {
        message = packet + 4 + i;
        type = message[0];
        if(type == MESSAGE_PAD1) {
            debugf(BABEL_DEBUG_COMMON,"Received pad1 from %s on %s.",
                   format_address(from), ifp->name);
            i++;
            continue;
        }
        len = message[1];

        if(type == MESSAGE_PADN) {
            debugf(BABEL_DEBUG_COMMON,"Received pad%d from %s on %s.",
                   len, format_address(from), ifp->name);
        } else if(type == MESSAGE_ACK_REQ) {
            unsigned short nonce, interval;
            DO_NTOHS(nonce, message + 4);
            DO_NTOHS(interval, message + 6);
            debugf(BABEL_DEBUG_COMMON,"Received ack-req (%04X %d) from %s on %s.",
                   nonce, interval, format_address(from), ifp->name);
            send_ack(neigh, nonce, interval);
        } else if(type == MESSAGE_ACK) {
            debugf(BABEL_DEBUG_COMMON,"Received ack from %s on %s.",
                   format_address(from), ifp->name);
            /* Nothing right now */
        } else if(type == MESSAGE_HELLO) {
            unsigned short seqno, interval;
            int changed;
            unsigned int timestamp = 0;
            DO_NTOHS(seqno, message + 4);
            DO_NTOHS(interval, message + 6);
            debugf(BABEL_DEBUG_COMMON,"Received hello %d (%d) from %s on %s.",
                   seqno, interval,
                   format_address(from), ifp->name);
            changed = update_neighbour(neigh, seqno, interval);
            update_neighbour_metric(neigh, changed);
            if(interval > 0)
                /* Multiply by 3/2 to allow hellos to expire. */
                schedule_neighbours_check(interval * 15, 0);
            /* Sub-TLV handling. */
            if(len > 8) {
                if(parse_hello_subtlv(message + 8, len - 6, &timestamp) > 0) {
                    neigh->hello_send_us = timestamp;
                    neigh->hello_rtt_receive_time = babel_now;
                    have_hello_rtt = 1;
                }
            }
        } else if(type == MESSAGE_IHU) {
            unsigned short txcost, interval;
            unsigned char address[16];
            int rc;
            DO_NTOHS(txcost, message + 4);
            DO_NTOHS(interval, message + 6);
            rc = network_address(message[2], message + 8, len - 6, address);
            if(rc < 0) goto fail;
            debugf(BABEL_DEBUG_COMMON,"Received ihu %d (%d) from %s on %s for %s.",
                   txcost, interval,
                   format_address(from), ifp->name,
                   format_address(address));
            if(message[2] == 0 || is_interface_ll_address(ifp, address)) {
                int changed = txcost != neigh->txcost;
                neigh->txcost = txcost;
                neigh->ihu_time = babel_now;
                neigh->ihu_interval = interval;
                update_neighbour_metric(neigh, changed);
                if(interval > 0)
                    /* Multiply by 3/2 to allow neighbours to expire. */
                    schedule_neighbours_check(interval * 45, 0);
                /* RTT sub-TLV. */
                if(len > 10 + rc)
                    parse_ihu_subtlv(message + 8 + rc, len - 6 - rc,
                                     &hello_send_us, &hello_rtt_receive_time);
            }
        } else if(type == MESSAGE_ROUTER_ID) {
            memcpy(router_id, message + 4, 8);
            have_router_id = 1;
            debugf(BABEL_DEBUG_COMMON,"Received router-id %s from %s on %s.",
                   format_eui64(router_id), format_address(from), ifp->name);
        } else if(type == MESSAGE_NH) {
            unsigned char nh[16];
            int rc;
            rc = network_address(message[2], message + 4, len - 2,
                                 nh);
            if(rc < 0) {
                have_v4_nh = 0;
                have_v6_nh = 0;
                goto fail;
            }
            debugf(BABEL_DEBUG_COMMON,"Received nh %s (%d) from %s on %s.",
                   format_address(nh), message[2],
                   format_address(from), ifp->name);
            if(message[2] == 1) {
                memcpy(v4_nh, nh, 16);
                have_v4_nh = 1;
            } else {
                memcpy(v6_nh, nh, 16);
                have_v6_nh = 1;
            }
        } else if(type == MESSAGE_UPDATE) {
            unsigned char prefix[16], *nh;
            unsigned char plen;
            unsigned char channels[DIVERSITY_HOPS];
            unsigned short interval, seqno, metric;
            int rc, parsed_len;
            DO_NTOHS(interval, message + 6);
            DO_NTOHS(seqno, message + 8);
            DO_NTOHS(metric, message + 10);
            if(message[5] == 0 ||
               (message[2] == 1 ? have_v4_prefix : have_v6_prefix))
                rc = network_prefix(message[2], message[4], message[5],
                                    message + 12,
                                    message[2] == 1 ? v4_prefix : v6_prefix,
                                    len - 10, prefix);
            else
                rc = -1;
            if(rc < 0) {
                if(message[3] & 0x80)
                    have_v4_prefix = have_v6_prefix = 0;
                goto fail;
            }
            parsed_len = 10 + rc;

            plen = message[4] + (message[2] == 1 ? 96 : 0);

            if(message[3] & 0x80) {
                if(message[2] == 1) {
                    memcpy(v4_prefix, prefix, 16);
                    have_v4_prefix = 1;
                } else {
                    memcpy(v6_prefix, prefix, 16);
                    have_v6_prefix = 1;
                }
            }
            if(message[3] & 0x40) {
                if(message[2] == 1) {
                    memset(router_id, 0, 4);
                    memcpy(router_id + 4, prefix + 12, 4);
                } else {
                    memcpy(router_id, prefix + 8, 8);
                }
                have_router_id = 1;
            }
            if(!have_router_id && message[2] != 0) {
                flog_err(EC_BABEL_PACKET,
			  "Received prefix with no router id.");
                goto fail;
            }
            debugf(BABEL_DEBUG_COMMON,"Received update%s%s for %s from %s on %s.",
                   (message[3] & 0x80) ? "/prefix" : "",
                   (message[3] & 0x40) ? "/id" : "",
                   format_prefix(prefix, plen),
                   format_address(from), ifp->name);

            if(message[2] == 0) {
                if(metric < 0xFFFF) {
                    flog_err(EC_BABEL_PACKET,
			      "Received wildcard update with finite metric.");
                    goto done;
                }
                retract_neighbour_routes(neigh);
                goto done;
            } else if(message[2] == 1) {
                if(!have_v4_nh)
                    goto fail;
                nh = v4_nh;
            } else if(have_v6_nh) {
                nh = v6_nh;
            } else {
                nh = neigh->address;
            }

            if(message[2] == 1) {
                if(!babel_get_if_nfo(ifp)->ipv4)
                    goto done;
            }

            if((babel_get_if_nfo(ifp)->flags & BABEL_IF_FARAWAY)) {
                channels[0] = 0;
            } else {
                /* This will be overwritten by parse_update_subtlv below. */
                if(metric < 256) {
                    /* Assume non-interfering (wired) link. */
                    channels[0] = 0;
                } else {
                    /* Assume interfering. */
                    channels[0] = BABEL_IF_CHANNEL_INTERFERING;
                    channels[1] = 0;
                }

                if(parsed_len < len)
                    parse_update_subtlv(message + 2 + parsed_len,
                                        len - parsed_len, channels);
            }

            update_route(router_id, prefix, plen, seqno, metric, interval,
                         neigh, nh,
                         channels, channels_len(channels));
        } else if(type == MESSAGE_REQUEST) {
            unsigned char prefix[16], plen;
            int rc;
            rc = network_prefix(message[2], message[3], 0,
                                message + 4, NULL, len - 2, prefix);
            if(rc < 0) goto fail;
            plen = message[3] + (message[2] == 1 ? 96 : 0);
            debugf(BABEL_DEBUG_COMMON,"Received request for %s from %s on %s.",
                   message[2] == 0 ? "any" : format_prefix(prefix, plen),
                   format_address(from), ifp->name);
            if(message[2] == 0) {
                struct babel_interface *neigh_ifp =babel_get_if_nfo(neigh->ifp);
                /* If a neighbour is requesting a full route dump from us,
                   we might as well send it an IHU. */
                send_ihu(neigh, NULL);
                /* Since nodes send wildcard requests on boot, booting
                   a large number of nodes at the same time may cause an
                   update storm.  Ignore a wildcard request that happens
                   shortly after we sent a full update. */
                if(neigh_ifp->last_update_time <
                   (time_t)(babel_now.tv_sec -
                            MAX(neigh_ifp->hello_interval / 100, 1)))
                    send_update(neigh->ifp, 0, NULL, 0);
            } else {
                send_update(neigh->ifp, 0, prefix, plen);
            }
        } else if(type == MESSAGE_MH_REQUEST) {
            unsigned char prefix[16], plen;
            unsigned short seqno;
            int rc;
            DO_NTOHS(seqno, message + 4);
            rc = network_prefix(message[2], message[3], 0,
                                message + 16, NULL, len - 14, prefix);
            if(rc < 0) goto fail;
            plen = message[3] + (message[2] == 1 ? 96 : 0);
            debugf(BABEL_DEBUG_COMMON,"Received request (%d) for %s from %s on %s (%s, %d).",
                   message[6],
                   format_prefix(prefix, plen),
                   format_address(from), ifp->name,
                   format_eui64(message + 8), seqno);
            handle_request(neigh, prefix, plen, message[6],
                           seqno, message + 8);
        } else {
            debugf(BABEL_DEBUG_COMMON,"Received unknown packet type %d from %s on %s.",
                   type, format_address(from), ifp->name);
        }
    done:
        i += len + 2;
        continue;

    fail:
        flog_err(EC_BABEL_PACKET,
		  "Couldn't parse packet (%d, %d) from %s on %s.",
                  message[0], message[1], format_address(from), ifp->name);
        goto done;
    }

    /* We can calculate the RTT to this neighbour. */
    if(have_hello_rtt && hello_send_us && hello_rtt_receive_time) {
        int remote_waiting_us, local_waiting_us;
        unsigned int rtt, smoothed_rtt;
        unsigned int old_rttcost;
        int changed = 0;
        remote_waiting_us = neigh->hello_send_us - hello_rtt_receive_time;
        local_waiting_us = time_us(neigh->hello_rtt_receive_time) -
            hello_send_us;

        /* Sanity checks (validity window of 10 minutes). */
        if(remote_waiting_us < 0 || local_waiting_us < 0 ||
           remote_waiting_us > 600000000 || local_waiting_us > 600000000)
            return;

        rtt = MAX(0, local_waiting_us - remote_waiting_us);
        debugf(BABEL_DEBUG_COMMON, "RTT to %s on %s sample result: %d us.",
               format_address(from), ifp->name, rtt);

        old_rttcost = neighbour_rttcost(neigh);
        if (valid_rtt(neigh)) {
            /* Running exponential average. */
            smoothed_rtt = (babel_ifp->rtt_decay * rtt +
			    (256 - babel_ifp->rtt_decay) * neigh->rtt);
            /* Rounding (up or down) to get closer to the sample. */
            neigh->rtt = (neigh->rtt >= rtt) ? smoothed_rtt / 256 :
                (smoothed_rtt + 255) / 256;
        } else {
            /* We prefer to be conservative with new neighbours
               (higher RTT) */
            assert(rtt <= 0x7FFFFFFF);
            neigh->rtt = 2*rtt;
        }
        changed = (neighbour_rttcost(neigh) == old_rttcost ? 0 : 1);
        update_neighbour_metric(neigh, changed);
        neigh->rtt_time = babel_now;
    }
    return;
}