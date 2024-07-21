miniflow_extract(struct dp_packet *packet, struct miniflow *dst)
{
    const struct pkt_metadata *md = &packet->md;
    const void *data = dp_packet_data(packet);
    size_t size = dp_packet_size(packet);
    uint64_t *values = miniflow_values(dst);
    struct mf_ctx mf = { FLOWMAP_EMPTY_INITIALIZER, values,
                         values + FLOW_U64S };
    const char *l2;
    ovs_be16 dl_type;
    uint8_t nw_frag, nw_tos, nw_ttl, nw_proto;

    /* Metadata. */
    if (flow_tnl_dst_is_set(&md->tunnel)) {
        miniflow_push_words(mf, tunnel, &md->tunnel,
                            offsetof(struct flow_tnl, metadata) /
                            sizeof(uint64_t));

        if (!(md->tunnel.flags & FLOW_TNL_F_UDPIF)) {
            if (md->tunnel.metadata.present.map) {
                miniflow_push_words(mf, tunnel.metadata, &md->tunnel.metadata,
                                    sizeof md->tunnel.metadata /
                                    sizeof(uint64_t));
            }
        } else {
            if (md->tunnel.metadata.present.len) {
                miniflow_push_words(mf, tunnel.metadata.present,
                                    &md->tunnel.metadata.present, 1);
                miniflow_push_words(mf, tunnel.metadata.opts.gnv,
                                    md->tunnel.metadata.opts.gnv,
                                    DIV_ROUND_UP(md->tunnel.metadata.present.len,
                                                 sizeof(uint64_t)));
            }
        }
    }
    if (md->skb_priority || md->pkt_mark) {
        miniflow_push_uint32(mf, skb_priority, md->skb_priority);
        miniflow_push_uint32(mf, pkt_mark, md->pkt_mark);
    }
    miniflow_push_uint32(mf, dp_hash, md->dp_hash);
    miniflow_push_uint32(mf, in_port, odp_to_u32(md->in_port.odp_port));
    if (md->recirc_id || md->ct_state) {
        miniflow_push_uint32(mf, recirc_id, md->recirc_id);
        miniflow_push_uint16(mf, ct_state, md->ct_state);
        miniflow_push_uint16(mf, ct_zone, md->ct_zone);
    }

    if (md->ct_state) {
        miniflow_push_uint32(mf, ct_mark, md->ct_mark);
        miniflow_pad_to_64(mf, pad1);

        if (!ovs_u128_is_zero(&md->ct_label)) {
            miniflow_push_words(mf, ct_label, &md->ct_label,
                                sizeof md->ct_label / sizeof(uint64_t));
        }
    }

    /* Initialize packet's layer pointer and offsets. */
    l2 = data;
    dp_packet_reset_offsets(packet);

    /* Must have full Ethernet header to proceed. */
    if (OVS_UNLIKELY(size < sizeof(struct eth_header))) {
        goto out;
    } else {
        ovs_be16 vlan_tci;

        /* Link layer. */
        ASSERT_SEQUENTIAL(dl_dst, dl_src);
        miniflow_push_macs(mf, dl_dst, data);
        /* dl_type, vlan_tci. */
        vlan_tci = parse_vlan(&data, &size);
        dl_type = parse_ethertype(&data, &size);
        miniflow_push_be16(mf, dl_type, dl_type);
        miniflow_push_be16(mf, vlan_tci, vlan_tci);
    }

    /* Parse mpls. */
    if (OVS_UNLIKELY(eth_type_mpls(dl_type))) {
        int count;
        const void *mpls = data;

        packet->l2_5_ofs = (char *)data - l2;
        count = parse_mpls(&data, &size);
        miniflow_push_words_32(mf, mpls_lse, mpls, count);
    }

    /* Network layer. */
    packet->l3_ofs = (char *)data - l2;

    nw_frag = 0;
    if (OVS_LIKELY(dl_type == htons(ETH_TYPE_IP))) {
        const struct ip_header *nh = data;
        int ip_len;
        uint16_t tot_len;

        if (OVS_UNLIKELY(size < IP_HEADER_LEN)) {
            goto out;
        }
        ip_len = IP_IHL(nh->ip_ihl_ver) * 4;

        if (OVS_UNLIKELY(ip_len < IP_HEADER_LEN)) {
            goto out;
        }
        if (OVS_UNLIKELY(size < ip_len)) {
            goto out;
        }
        tot_len = ntohs(nh->ip_tot_len);
        if (OVS_UNLIKELY(tot_len > size || ip_len > tot_len)) {
            goto out;
        }
        if (OVS_UNLIKELY(size - tot_len > UINT8_MAX)) {
            goto out;
        }
        dp_packet_set_l2_pad_size(packet, size - tot_len);
        size = tot_len;   /* Never pull padding. */

        /* Push both source and destination address at once. */
        miniflow_push_words(mf, nw_src, &nh->ip_src, 1);

        miniflow_push_be32(mf, ipv6_label, 0); /* Padding for IPv4. */

        nw_tos = nh->ip_tos;
        nw_ttl = nh->ip_ttl;
        nw_proto = nh->ip_proto;
        if (OVS_UNLIKELY(IP_IS_FRAGMENT(nh->ip_frag_off))) {
            nw_frag = FLOW_NW_FRAG_ANY;
            if (nh->ip_frag_off & htons(IP_FRAG_OFF_MASK)) {
                nw_frag |= FLOW_NW_FRAG_LATER;
            }
        }
        data_pull(&data, &size, ip_len);
    } else if (dl_type == htons(ETH_TYPE_IPV6)) {
        const struct ovs_16aligned_ip6_hdr *nh;
        ovs_be32 tc_flow;
        uint16_t plen;

        if (OVS_UNLIKELY(size < sizeof *nh)) {
            goto out;
        }
        nh = data_pull(&data, &size, sizeof *nh);

        plen = ntohs(nh->ip6_plen);
        if (OVS_UNLIKELY(plen > size)) {
            goto out;
        }
        /* Jumbo Payload option not supported yet. */
        if (OVS_UNLIKELY(size - plen > UINT8_MAX)) {
            goto out;
        }
        dp_packet_set_l2_pad_size(packet, size - plen);
        size = plen;   /* Never pull padding. */

        miniflow_push_words(mf, ipv6_src, &nh->ip6_src,
                            sizeof nh->ip6_src / 8);
        miniflow_push_words(mf, ipv6_dst, &nh->ip6_dst,
                            sizeof nh->ip6_dst / 8);

        tc_flow = get_16aligned_be32(&nh->ip6_flow);
        nw_tos = ntohl(tc_flow) >> 20;
        nw_ttl = nh->ip6_hlim;
        nw_proto = nh->ip6_nxt;

        while (1) {
            if (OVS_LIKELY((nw_proto != IPPROTO_HOPOPTS)
                           && (nw_proto != IPPROTO_ROUTING)
                           && (nw_proto != IPPROTO_DSTOPTS)
                           && (nw_proto != IPPROTO_AH)
                           && (nw_proto != IPPROTO_FRAGMENT))) {
                /* It's either a terminal header (e.g., TCP, UDP) or one we
                 * don't understand.  In either case, we're done with the
                 * packet, so use it to fill in 'nw_proto'. */
                break;
            }

            /* We only verify that at least 8 bytes of the next header are
             * available, but many of these headers are longer.  Ensure that
             * accesses within the extension header are within those first 8
             * bytes. All extension headers are required to be at least 8
             * bytes. */
            if (OVS_UNLIKELY(size < 8)) {
                goto out;
            }

            if ((nw_proto == IPPROTO_HOPOPTS)
                || (nw_proto == IPPROTO_ROUTING)
                || (nw_proto == IPPROTO_DSTOPTS)) {
                /* These headers, while different, have the fields we care
                 * about in the same location and with the same
                 * interpretation. */
                const struct ip6_ext *ext_hdr = data;
                nw_proto = ext_hdr->ip6e_nxt;
                if (OVS_UNLIKELY(!data_try_pull(&data, &size,
                                                (ext_hdr->ip6e_len + 1) * 8))) {
                    goto out;
                }
            } else if (nw_proto == IPPROTO_AH) {
                /* A standard AH definition isn't available, but the fields
                 * we care about are in the same location as the generic
                 * option header--only the header length is calculated
                 * differently. */
                const struct ip6_ext *ext_hdr = data;
                nw_proto = ext_hdr->ip6e_nxt;
                if (OVS_UNLIKELY(!data_try_pull(&data, &size,
                                                (ext_hdr->ip6e_len + 2) * 4))) {
                    goto out;
                }
            } else if (nw_proto == IPPROTO_FRAGMENT) {
                const struct ovs_16aligned_ip6_frag *frag_hdr = data;

                nw_proto = frag_hdr->ip6f_nxt;
                if (!data_try_pull(&data, &size, sizeof *frag_hdr)) {
                    goto out;
                }

                /* We only process the first fragment. */
                if (frag_hdr->ip6f_offlg != htons(0)) {
                    nw_frag = FLOW_NW_FRAG_ANY;
                    if ((frag_hdr->ip6f_offlg & IP6F_OFF_MASK) != htons(0)) {
                        nw_frag |= FLOW_NW_FRAG_LATER;
                        nw_proto = IPPROTO_FRAGMENT;
                        break;
                    }
                }
            }
        }

        /* This needs to be after the parse_ipv6_ext_hdrs__() call because it
         * leaves the nw_frag word uninitialized. */
        ASSERT_SEQUENTIAL(ipv6_label, nw_frag);
        ovs_be32 label = tc_flow & htonl(IPV6_LABEL_MASK);
        miniflow_push_be32(mf, ipv6_label, label);
    } else {
        if (dl_type == htons(ETH_TYPE_ARP) ||
            dl_type == htons(ETH_TYPE_RARP)) {
            struct eth_addr arp_buf[2];
            const struct arp_eth_header *arp = (const struct arp_eth_header *)
                data_try_pull(&data, &size, ARP_ETH_HEADER_LEN);

            if (OVS_LIKELY(arp) && OVS_LIKELY(arp->ar_hrd == htons(1))
                && OVS_LIKELY(arp->ar_pro == htons(ETH_TYPE_IP))
                && OVS_LIKELY(arp->ar_hln == ETH_ADDR_LEN)
                && OVS_LIKELY(arp->ar_pln == 4)) {
                miniflow_push_be32(mf, nw_src,
                                   get_16aligned_be32(&arp->ar_spa));
                miniflow_push_be32(mf, nw_dst,
                                   get_16aligned_be32(&arp->ar_tpa));

                /* We only match on the lower 8 bits of the opcode. */
                if (OVS_LIKELY(ntohs(arp->ar_op) <= 0xff)) {
                    miniflow_push_be32(mf, ipv6_label, 0); /* Pad with ARP. */
                    miniflow_push_be32(mf, nw_frag, htonl(ntohs(arp->ar_op)));
                }

                /* Must be adjacent. */
                ASSERT_SEQUENTIAL(arp_sha, arp_tha);

                arp_buf[0] = arp->ar_sha;
                arp_buf[1] = arp->ar_tha;
                miniflow_push_macs(mf, arp_sha, arp_buf);
                miniflow_pad_to_64(mf, tcp_flags);
            }
        }
        goto out;
    }

    packet->l4_ofs = (char *)data - l2;
    miniflow_push_be32(mf, nw_frag,
                       BYTES_TO_BE32(nw_frag, nw_tos, nw_ttl, nw_proto));

    if (OVS_LIKELY(!(nw_frag & FLOW_NW_FRAG_LATER))) {
        if (OVS_LIKELY(nw_proto == IPPROTO_TCP)) {
            if (OVS_LIKELY(size >= TCP_HEADER_LEN)) {
                const struct tcp_header *tcp = data;

                miniflow_push_be32(mf, arp_tha.ea[2], 0);
                miniflow_push_be32(mf, tcp_flags,
                                   TCP_FLAGS_BE32(tcp->tcp_ctl));
                miniflow_push_be16(mf, tp_src, tcp->tcp_src);
                miniflow_push_be16(mf, tp_dst, tcp->tcp_dst);
                miniflow_pad_to_64(mf, igmp_group_ip4);
            }
        } else if (OVS_LIKELY(nw_proto == IPPROTO_UDP)) {
            if (OVS_LIKELY(size >= UDP_HEADER_LEN)) {
                const struct udp_header *udp = data;

                miniflow_push_be16(mf, tp_src, udp->udp_src);
                miniflow_push_be16(mf, tp_dst, udp->udp_dst);
                miniflow_pad_to_64(mf, igmp_group_ip4);
            }
        } else if (OVS_LIKELY(nw_proto == IPPROTO_SCTP)) {
            if (OVS_LIKELY(size >= SCTP_HEADER_LEN)) {
                const struct sctp_header *sctp = data;

                miniflow_push_be16(mf, tp_src, sctp->sctp_src);
                miniflow_push_be16(mf, tp_dst, sctp->sctp_dst);
                miniflow_pad_to_64(mf, igmp_group_ip4);
            }
        } else if (OVS_LIKELY(nw_proto == IPPROTO_ICMP)) {
            if (OVS_LIKELY(size >= ICMP_HEADER_LEN)) {
                const struct icmp_header *icmp = data;

                miniflow_push_be16(mf, tp_src, htons(icmp->icmp_type));
                miniflow_push_be16(mf, tp_dst, htons(icmp->icmp_code));
                miniflow_pad_to_64(mf, igmp_group_ip4);
            }
        } else if (OVS_LIKELY(nw_proto == IPPROTO_IGMP)) {
            if (OVS_LIKELY(size >= IGMP_HEADER_LEN)) {
                const struct igmp_header *igmp = data;

                miniflow_push_be16(mf, tp_src, htons(igmp->igmp_type));
                miniflow_push_be16(mf, tp_dst, htons(igmp->igmp_code));
                miniflow_push_be32(mf, igmp_group_ip4,
                                   get_16aligned_be32(&igmp->group));
            }
        } else if (OVS_LIKELY(nw_proto == IPPROTO_ICMPV6)) {
            if (OVS_LIKELY(size >= sizeof(struct icmp6_hdr))) {
                const struct in6_addr *nd_target = NULL;
                struct eth_addr arp_buf[2] = { { { { 0 } } } };
                const struct icmp6_hdr *icmp = data_pull(&data, &size,
                                                         sizeof *icmp);
                parse_icmpv6(&data, &size, icmp, &nd_target, arp_buf);
                if (nd_target) {
                    miniflow_push_words(mf, nd_target, nd_target,
                                        sizeof *nd_target / sizeof(uint64_t));
                }
                miniflow_push_macs(mf, arp_sha, arp_buf);
                miniflow_pad_to_64(mf, tcp_flags);
                miniflow_push_be16(mf, tp_src, htons(icmp->icmp6_type));
                miniflow_push_be16(mf, tp_dst, htons(icmp->icmp6_code));
                miniflow_pad_to_64(mf, igmp_group_ip4);
            }
        }
    }
 out:
    dst->map = mf.map;
}