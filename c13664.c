add_tree_first_ipv6(const u_char *data, const int len, const int datalink)
{
    tcpr_tree_t *newnode, *findnode;
    const u_char *packet = data;
    uint32_t _U_ vlan_offset;
    uint32_t pkt_len = len;
    uint16_t ether_type;
    ipv6_hdr_t ip6_hdr;
    uint32_t l2offset;
    uint32_t l2len;
    int res;

    assert(packet);

    res = get_l2len_protocol(packet,
                             pkt_len,
                             datalink,
                             &ether_type,
                             &l2len,
                             &l2offset,
                             &vlan_offset);

    if (res == -1 || len < (TCPR_ETH_H + TCPR_IPV6_H)) {
        errx(-1, "Capture length %d too small for IPv6 parsing", len);
        return;
    }

    packet += l2offset;
    l2len -= l2offset;
    pkt_len -= l2offset;

    /*
     * first add/find the source IP/client
     */
    newnode = new_tree();
    
    /* prevent issues with byte alignment, must memcpy */
    memcpy(&ip6_hdr, (packet + TCPR_ETH_H), TCPR_IPV6_H);

    /* copy over the source ip, and values to guarantee this a client */
    newnode->family = AF_INET6;
    newnode->u.ip6 = ip6_hdr.ip_src;
    newnode->type = DIR_CLIENT;
    newnode->client_cnt = 1000;
    findnode = RB_FIND(tcpr_data_tree_s, &treeroot, newnode);

    /* if we didn't find it, add it to the tree, else free it */
    if (findnode == NULL) {
        RB_INSERT(tcpr_data_tree_s, &treeroot, newnode);
    } else {
        safe_free(newnode);
    }

    /*
     * now add/find the destination IP/server
     */
    newnode = new_tree();
    memcpy(&ip6_hdr, (packet + TCPR_ETH_H), TCPR_IPV6_H);

    newnode->family = AF_INET6;
    newnode->u.ip6 = ip6_hdr.ip_dst;
    newnode->type = DIR_SERVER;
    newnode->server_cnt = 1000;
    findnode = RB_FIND(tcpr_data_tree_s, &treeroot, newnode);

    if (findnode == NULL) {
        RB_INSERT(tcpr_data_tree_s, &treeroot, newnode);
    } else {
        safe_free(newnode);
    }
}