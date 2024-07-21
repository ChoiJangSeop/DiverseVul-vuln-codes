flow_entry_type_t flow_decode(flow_hash_table_t *fht, const struct pcap_pkthdr *pkthdr,
        const u_char *pktdata, const int datalink, const int expiry)
{
    uint32_t pkt_len = pkthdr->caplen;
    const u_char *packet = pktdata;
    uint32_t _U_ vlan_offset;
    uint16_t ether_type = 0;
    ipv4_hdr_t *ip_hdr = NULL;
    ipv6_hdr_t *ip6_hdr = NULL;
    tcp_hdr_t *tcp_hdr;
    udp_hdr_t *udp_hdr;
    icmpv4_hdr_t *icmp_hdr;
    flow_entry_data_t entry;
    uint32_t l2len = 0;
    uint32_t l2offset;
    uint8_t protocol;
    uint32_t hash;
    int ip_len;
    int res;

    assert(fht);
    assert(packet);

    /*
     * extract the 5-tuple and populate the entry data
     */

    memset(&entry, 0, sizeof(entry));

    res = get_l2len_protocol(packet,
                             pkt_len,
                             datalink,
                             &ether_type,
                             &l2len,
                             &l2offset,
                             &vlan_offset);

    if (res == -1) {
        warnx("Unable to process unsupported DLT type: %s (0x%x)",
              pcap_datalink_val_to_description(datalink), datalink);
        return FLOW_ENTRY_INVALID;
    }

    packet += l2offset;
    l2len -= l2offset;
    pkt_len -= l2offset;

    assert(l2len > 0);

    if (ether_type == ETHERTYPE_IP) {
        if (pkt_len < l2len + sizeof(ipv4_hdr_t))
                return FLOW_ENTRY_INVALID;

        ip_hdr = (ipv4_hdr_t *)(packet + l2len);

        if (ip_hdr->ip_v != 4)
            return FLOW_ENTRY_NON_IP;

        ip_len = ip_hdr->ip_hl * 4;
        protocol = ip_hdr->ip_p;
        entry.src_ip.in = ip_hdr->ip_src;
        entry.dst_ip.in = ip_hdr->ip_dst;
    } else if (ether_type == ETHERTYPE_IP6) {
        if (pkt_len < l2len + sizeof(ipv6_hdr_t))
                return FLOW_ENTRY_INVALID;

        if ((packet[0] >> 4) != 6)
            return FLOW_ENTRY_NON_IP;

        ip6_hdr = (ipv6_hdr_t *)(packet + l2len);
        ip_len = sizeof(*ip6_hdr);
        protocol = ip6_hdr->ip_nh;

        if (protocol == 0) {
            struct tcpr_ipv6_ext_hdr_base *ext = (struct tcpr_ipv6_ext_hdr_base *)(ip6_hdr + 1);
            ip_len += (ext->ip_len + 1) * 8;
            protocol = ext->ip_nh;
        }
        memcpy(&entry.src_ip.in6, &ip6_hdr->ip_src, sizeof(entry.src_ip.in6));
        memcpy(&entry.dst_ip.in6, &ip6_hdr->ip_dst, sizeof(entry.dst_ip.in6));
    } else {
        return FLOW_ENTRY_NON_IP;
    }

    entry.protocol = protocol;

    switch (protocol) {
    case IPPROTO_UDP:
        if (pkt_len < (l2len + ip_len + sizeof(udp_hdr_t)))
            return FLOW_ENTRY_INVALID;
        udp_hdr = (udp_hdr_t*)(packet + ip_len + l2len);
        entry.src_port = udp_hdr->uh_sport;
        entry.dst_port = udp_hdr->uh_dport;
        break;

    case IPPROTO_TCP:
        if (pkt_len < (l2len + ip_len + sizeof(tcp_hdr_t)))
            return FLOW_ENTRY_INVALID;
        tcp_hdr = (tcp_hdr_t*)(packet + ip_len + l2len);
        entry.src_port = tcp_hdr->th_sport;
        entry.dst_port = tcp_hdr->th_dport;
        break;

    case IPPROTO_ICMP:
    case IPPROTO_ICMPV6:
        if (pkt_len < (l2len + ip_len + sizeof(icmpv4_hdr_t)))
            return FLOW_ENTRY_INVALID;
        icmp_hdr = (icmpv4_hdr_t*)(packet + ip_len + l2len);
        entry.src_port = icmp_hdr->icmp_type;
        entry.dst_port = icmp_hdr->icmp_code;
        break;
    }

    /* hash the 5-tuple */
    hash = hash_func(&entry, sizeof(entry));

    return hash_put_data(fht, hash, &entry, &pkthdr->ts, expiry);
}