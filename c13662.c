packet2tree(const u_char * data, const int len, int datalink)
{
    const u_char *packet = data;
    uint32_t _U_ vlan_offset;
    ssize_t pkt_len = len;
    tcpr_tree_t *node = NULL;
    eth_hdr_t *eth_hdr = NULL;
    ipv4_hdr_t ip_hdr;
    ipv6_hdr_t ip6_hdr;
    tcp_hdr_t tcp_hdr;
    udp_hdr_t udp_hdr;
    icmpv4_hdr_t icmp_hdr;
    dnsv4_hdr_t dnsv4_hdr;
    u_int16_t ether_type;
    uint32_t l2offset;
    u_char proto = 0;
    uint32_t l2len;
    int hl = 0;
    int res;

#ifdef DEBUG
    char srcip[INET6_ADDRSTRLEN];
#endif

    res = get_l2len_protocol(packet,
                             pkt_len,
                             datalink,
                             &ether_type,
                             &l2len,
                             &l2offset,
                             &vlan_offset);

    if (res == -1 || len < (int)sizeof(*eth_hdr))
        goto len_error;

    node = new_tree();

    packet += l2offset;
    l2len -= l2offset;
    pkt_len -= l2offset;

    assert(l2len > 0);

    eth_hdr = (eth_hdr_t *) (packet);
    if (ether_type == ETHERTYPE_IP) {
        if (pkt_len < (TCPR_ETH_H + hl + TCPR_IPV4_H)) {
            safe_free(node);
            errx(-1, "packet capture length %d too small for IPv4 processing",
                    len);
            return NULL;
        }

        if (pkt_len < TCPR_ETH_H + TCPR_IPV4_H + hl) {
            goto len_error;
        }

        memcpy(&ip_hdr, packet + TCPR_ETH_H + hl, TCPR_IPV4_H);

        node->family = AF_INET;
        node->u.ip = ip_hdr.ip_src.s_addr;
        proto = ip_hdr.ip_p;
        hl += ip_hdr.ip_hl * 4;

#ifdef DEBUG
        strlcpy(srcip, get_addr2name4(ip_hdr.ip_src.s_addr,
                    RESOLVE), 16);
#endif
    } else if (ether_type == ETHERTYPE_IP6) {
        if (pkt_len < (TCPR_ETH_H + hl + TCPR_IPV6_H)) {
            safe_free(node);
            errx(-1, "packet capture length %d too small for IPv6 processing",
                    len);
            return NULL;
        }

        if (pkt_len < TCPR_ETH_H + TCPR_IPV6_H + hl) {
            goto len_error;
        }

        memcpy(&ip6_hdr, packet + TCPR_ETH_H + hl, TCPR_IPV6_H);

        node->family = AF_INET6;
        node->u.ip6 = ip6_hdr.ip_src;
        proto = ip6_hdr.ip_nh;
        hl += TCPR_IPV6_H;

#ifdef DEBUG
        strlcpy(srcip, get_addr2name6(&ip6_hdr.ip_src, RESOLVE), INET6_ADDRSTRLEN);
#endif
    } else {
       dbgx(2,"Unrecognized ether_type (%x)", ether_type);
    }


    /* copy over the source mac */
    strlcpy((char *)node->mac, (char *)eth_hdr->ether_shost, sizeof(node->mac));

    /* 
     * TCP 
     */
    if (proto == IPPROTO_TCP) {

        dbgx(3, "%s uses TCP...  ", srcip);

        if (pkt_len < TCPR_ETH_H + TCPR_TCP_H + hl)
            goto len_error;

        /* memcpy it over to prevent alignment issues */
        memcpy(&tcp_hdr, (data + TCPR_ETH_H + hl), TCPR_TCP_H);

        /* ftp-data is going to skew our results so we ignore it */
        if (tcp_hdr.th_sport == 20)
            return (node);

        /* set TREE->type based on TCP flags */
        if (tcp_hdr.th_flags == TH_SYN) {
            node->type = DIR_CLIENT;
            dbg(3, "is a client");
        }
        else if (tcp_hdr.th_flags == (TH_SYN | TH_ACK)) {
            node->type = DIR_SERVER;
            dbg(3, "is a server");
        }
        else {
            dbg(3, "is an unknown");
        }

    }
    /* 
     * UDP 
     */
    else if (proto == IPPROTO_UDP) {
        if (pkt_len < TCPR_ETH_H + TCPR_UDP_H + hl)
            goto len_error;

        /* memcpy over to prevent alignment issues */
        memcpy(&udp_hdr, (data + TCPR_ETH_H + hl), TCPR_UDP_H);
        dbgx(3, "%s uses UDP...  ", srcip);

        switch (ntohs(udp_hdr.uh_dport)) {
        case 0x0035:           /* dns */
            if (pkt_len < TCPR_ETH_H + TCPR_UDP_H + TCPR_DNS_H + hl)
                goto len_error;

            /* prevent memory alignment issues */
            memcpy(&dnsv4_hdr,
                   (data + TCPR_ETH_H + hl + TCPR_UDP_H), TCPR_DNS_H);

            if (dnsv4_hdr.flags & DNS_QUERY_FLAG) {
                /* bit set, response */
                node->type = DIR_SERVER;

                dbg(3, "is a dns server");

            }
            else {
                /* bit not set, query */
                node->type = DIR_CLIENT;

                dbg(3, "is a dns client");
            }
            return (node);
            break;
        default:
            break;
        }

        switch (ntohs(udp_hdr.uh_sport)) {
        case 0x0035:           /* dns */
            if (pkt_len < TCPR_ETH_H + TCPR_UDP_H + TCPR_DNS_H + hl)
                goto len_error;

            /* prevent memory alignment issues */
            memcpy(&dnsv4_hdr,
                   (data + TCPR_ETH_H + hl + TCPR_UDP_H),
                   TCPR_DNS_H);

            if ((dnsv4_hdr.flags & 0x7FFFF) ^ DNS_QUERY_FLAG) {
                /* bit set, response */
                node->type = DIR_SERVER;
                dbg(3, "is a dns server");
            }
            else {
                /* bit not set, query */
                node->type = DIR_CLIENT;
                dbg(3, "is a dns client");
            }
            return (node);
            break;
        default:

            dbgx(3, "unknown UDP protocol: %hu->%hu", udp_hdr.uh_sport,
                udp_hdr.uh_dport);
            break;
        }
    }
    /* 
     * ICMP 
     */
    else if (proto == IPPROTO_ICMP) {
        if (pkt_len < TCPR_ETH_H + TCPR_ICMPV4_H + hl)
            goto len_error;

        /* prevent alignment issues */
        memcpy(&icmp_hdr, (data + TCPR_ETH_H + hl), TCPR_ICMPV4_H);

        dbgx(3, "%s uses ICMP...  ", srcip);

        /*
         * if port unreachable, then source == server, dst == client 
         */
        if ((icmp_hdr.icmp_type == ICMP_UNREACH) &&
            (icmp_hdr.icmp_code == ICMP_UNREACH_PORT)) {
            node->type = DIR_SERVER;
            dbg(3, "is a server with a closed port");
        }

    }

    return (node);

len_error:
    errx(-1, "packet capture length %d too small to process", len);
    return NULL;
}