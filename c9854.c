reverse_nat_packet(struct dp_packet *pkt, const struct conn *conn)
{
    char *tail = dp_packet_tail(pkt);
    uint8_t pad = dp_packet_l2_pad_size(pkt);
    struct conn_key inner_key;
    const char *inner_l4 = NULL;
    uint16_t orig_l3_ofs = pkt->l3_ofs;
    uint16_t orig_l4_ofs = pkt->l4_ofs;

    if (conn->key.dl_type == htons(ETH_TYPE_IP)) {
        struct ip_header *nh = dp_packet_l3(pkt);
        struct icmp_header *icmp = dp_packet_l4(pkt);
        struct ip_header *inner_l3 = (struct ip_header *) (icmp + 1);
        /* This call is already verified to succeed during the code path from
         * 'conn_key_extract()' which calls 'extract_l4_icmp()'. */
        extract_l3_ipv4(&inner_key, inner_l3, tail - ((char *)inner_l3) - pad,
                        &inner_l4, false);

        pkt->l3_ofs += (char *) inner_l3 - (char *) nh;
        pkt->l4_ofs += inner_l4 - (char *) icmp;

        if (conn->nat_info->nat_action & NAT_ACTION_SRC) {
            packet_set_ipv4_addr(pkt, &inner_l3->ip_src,
                                 conn->key.src.addr.ipv4_aligned);
        } else if (conn->nat_info->nat_action & NAT_ACTION_DST) {
            packet_set_ipv4_addr(pkt, &inner_l3->ip_dst,
                                 conn->key.dst.addr.ipv4_aligned);
        }
        reverse_pat_packet(pkt, conn);
        icmp->icmp_csum = 0;
        icmp->icmp_csum = csum(icmp, tail - (char *) icmp - pad);
    } else {
        struct ovs_16aligned_ip6_hdr *nh6 = dp_packet_l3(pkt);
        struct icmp6_error_header *icmp6 = dp_packet_l4(pkt);
        struct ovs_16aligned_ip6_hdr *inner_l3_6 =
            (struct ovs_16aligned_ip6_hdr *) (icmp6 + 1);
        /* This call is already verified to succeed during the code path from
         * 'conn_key_extract()' which calls 'extract_l4_icmp6()'. */
        extract_l3_ipv6(&inner_key, inner_l3_6,
                        tail - ((char *)inner_l3_6) - pad,
                        &inner_l4);
        pkt->l3_ofs += (char *) inner_l3_6 - (char *) nh6;
        pkt->l4_ofs += inner_l4 - (char *) icmp6;

        if (conn->nat_info->nat_action & NAT_ACTION_SRC) {
            packet_set_ipv6_addr(pkt, conn->key.nw_proto,
                                 inner_l3_6->ip6_src.be32,
                                 &conn->key.src.addr.ipv6_aligned,
                                 true);
        } else if (conn->nat_info->nat_action & NAT_ACTION_DST) {
            packet_set_ipv6_addr(pkt, conn->key.nw_proto,
                                 inner_l3_6->ip6_dst.be32,
                                 &conn->key.dst.addr.ipv6_aligned,
                                 true);
        }
        reverse_pat_packet(pkt, conn);
        icmp6->icmp6_base.icmp6_cksum = 0;
        icmp6->icmp6_base.icmp6_cksum = packet_csum_upperlayer6(nh6, icmp6,
            IPPROTO_ICMPV6, tail - (char *) icmp6 - pad);
    }
    pkt->l3_ofs = orig_l3_ofs;
    pkt->l4_ofs = orig_l4_ofs;
}