fast_edit_packet(struct pcap_pkthdr *pkthdr, u_char **pktdata,
        COUNTER iteration, bool cached, int datalink)
{
    uint32_t pkt_len = pkthdr->caplen;
    u_char *packet = *pktdata;
    ipv4_hdr_t *ip_hdr = NULL;
    ipv6_hdr_t *ip6_hdr = NULL;
    uint32_t src_ip, dst_ip;
    uint32_t src_ip_orig, dst_ip_orig;
    uint32_t _U_ vlan_offset;
    uint16_t ether_type;
    uint32_t l2offset;
    uint32_t l2len;
    int res;

    res = get_l2len_protocol(packet,
                             pkt_len,
                             datalink,
                             &ether_type,
                             &l2len,
                             &l2offset,
                             &vlan_offset);

    if (res < 0)
        return res;

    packet += l2offset;
    l2len -= l2offset;
    pkt_len -= l2offset;

    assert(l2len > 0);

    switch (ether_type) {
    case ETHERTYPE_IP:
        if (pkt_len < (bpf_u_int32)(l2len + sizeof(ipv4_hdr_t))) {
            dbgx(1, "IP packet too short for Unique IP feature: %u", pkthdr->caplen);
            return -1;
        }
        ip_hdr = (ipv4_hdr_t *)(packet + l2len);
        src_ip_orig = src_ip = ntohl(ip_hdr->ip_src.s_addr);
        dst_ip_orig = dst_ip = ntohl(ip_hdr->ip_dst.s_addr);
        break;

    case ETHERTYPE_IP6:
        if (pkt_len < (bpf_u_int32)(l2len + sizeof(ipv6_hdr_t))) {
            dbgx(1, "IP6 packet too short for Unique IP feature: %u", pkthdr->caplen);
            return -1;
        }
        ip6_hdr = (ipv6_hdr_t *)(packet + l2len);
        src_ip_orig = src_ip = ntohl(ip6_hdr->ip_src.__u6_addr.__u6_addr32[3]);
        dst_ip_orig = dst_ip = ntohl(ip6_hdr->ip_dst.__u6_addr.__u6_addr32[3]);
        break;

    default:
        return -1; /* non-IP or packet too short */
    }

    dbgx(2, "Layer 3 protocol type is: 0x%04x", ether_type);

    /* swap src/dst IP's in a manner that does not affect CRC */
    if ((!cached && dst_ip > src_ip) ||
            (cached && (dst_ip - iteration) > (src_ip - 1 - iteration))) {
        if (cached) {
            --src_ip;
            ++dst_ip;
        } else {
            src_ip -= iteration;
            dst_ip += iteration;
        }

        /* CRC compensations  for wrap conditions */
        if (src_ip > src_ip_orig && dst_ip > dst_ip_orig) {
            dbgx(1, "dst_ip > src_ip(" COUNTER_SPEC "): before(1) src_ip=0x%08x dst_ip=0x%08x", iteration, src_ip, dst_ip);
            --src_ip;
            dbgx(1, "dst_ip > src_ip(" COUNTER_SPEC "): after(1)  src_ip=0x%08x dst_ip=0x%08x", iteration, src_ip, dst_ip);
        } else if (dst_ip < dst_ip_orig && src_ip < src_ip_orig) {
            dbgx(1, "dst_ip > src_ip(" COUNTER_SPEC "): before(2) src_ip=0x%08x dst_ip=0x%08x", iteration, src_ip, dst_ip);
            ++dst_ip;
            dbgx(1, "dst_ip > src_ip(" COUNTER_SPEC "): after(2)  src_ip=0x%08x dst_ip=0x%08x", iteration, src_ip, dst_ip);
        }
    } else {
        if (cached) {
            ++src_ip;
            --dst_ip;
        } else {
            src_ip += iteration;
            dst_ip -= iteration;
        }

        /* CRC compensations  for wrap conditions */
        if (dst_ip > dst_ip_orig && src_ip > src_ip_orig) {
            dbgx(1, "src_ip > dst_ip(" COUNTER_SPEC "): before(1) dst_ip=0x%08x src_ip=0x%08x", iteration, dst_ip, src_ip);
            --dst_ip;
            dbgx(1, "src_ip > dst_ip(" COUNTER_SPEC "): after(1)  dst_ip=0x%08x src_ip=0x%08x", iteration, dst_ip, src_ip);
        } else if (src_ip < src_ip_orig && dst_ip < dst_ip_orig) {
            dbgx(1, "src_ip > dst_ip(" COUNTER_SPEC "): before(2) dst_ip=0x%08x src_ip=0x%08x", iteration, dst_ip, src_ip);
            ++src_ip;
            dbgx(1, "src_ip > dst_ip(" COUNTER_SPEC "): after(2)  dst_ip=0x%08x src_ip=0x%08x", iteration, dst_ip, src_ip);
        }
    }

    dbgx(1, "(" COUNTER_SPEC "): final src_ip=0x%08x dst_ip=0x%08x", iteration, src_ip, dst_ip);

    switch (ether_type) {
    case ETHERTYPE_IP:
        ip_hdr->ip_src.s_addr = htonl(src_ip);
        ip_hdr->ip_dst.s_addr = htonl(dst_ip);
        break;

    case ETHERTYPE_IP6:
        ip6_hdr->ip_src.__u6_addr.__u6_addr32[3] = htonl(src_ip);
        ip6_hdr->ip_dst.__u6_addr.__u6_addr32[3] = htonl(dst_ip);
        break;
    }

    return 0;
}