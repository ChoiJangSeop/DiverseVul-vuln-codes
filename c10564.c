void udp_input(register struct mbuf *m, int iphlen)
{
    Slirp *slirp = m->slirp;
    M_DUP_DEBUG(slirp, m, 0, 0);

    register struct ip *ip;
    register struct udphdr *uh;
    int len;
    struct ip save_ip;
    struct socket *so;
    struct sockaddr_storage lhost;
    struct sockaddr_in *lhost4;
    int ttl;

    DEBUG_CALL("udp_input");
    DEBUG_ARG("m = %p", m);
    DEBUG_ARG("iphlen = %d", iphlen);

    /*
     * Strip IP options, if any; should skip this,
     * make available to user, and use on returned packets,
     * but we don't yet have a way to check the checksum
     * with options still present.
     */
    if (iphlen > sizeof(struct ip)) {
        ip_stripoptions(m, (struct mbuf *)0);
        iphlen = sizeof(struct ip);
    }

    /*
     * Get IP and UDP header together in first mbuf.
     */
    ip = mtod(m, struct ip *);
    uh = (struct udphdr *)((char *)ip + iphlen);

    /*
     * Make mbuf data length reflect UDP length.
     * If not enough data to reflect UDP length, drop.
     */
    len = ntohs((uint16_t)uh->uh_ulen);

    if (ip->ip_len != len) {
        if (len > ip->ip_len) {
            goto bad;
        }
        m_adj(m, len - ip->ip_len);
        ip->ip_len = len;
    }

    /*
     * Save a copy of the IP header in case we want restore it
     * for sending an ICMP error message in response.
     */
    save_ip = *ip;
    save_ip.ip_len += iphlen; /* tcp_input subtracts this */

    /*
     * Checksum extended UDP header and data.
     */
    if (uh->uh_sum) {
        memset(&((struct ipovly *)ip)->ih_mbuf, 0, sizeof(struct mbuf_ptr));
        ((struct ipovly *)ip)->ih_x1 = 0;
        ((struct ipovly *)ip)->ih_len = uh->uh_ulen;
        if (cksum(m, len + sizeof(struct ip))) {
            goto bad;
        }
    }

    lhost.ss_family = AF_INET;
    lhost4 = (struct sockaddr_in *)&lhost;
    lhost4->sin_addr = ip->ip_src;
    lhost4->sin_port = uh->uh_sport;

    /*
     *  handle DHCP/BOOTP
     */
    if (ntohs(uh->uh_dport) == BOOTP_SERVER &&
        (ip->ip_dst.s_addr == slirp->vhost_addr.s_addr ||
         ip->ip_dst.s_addr == 0xffffffff)) {
        bootp_input(m);
        goto bad;
    }

    /*
     *  handle TFTP
     */
    if (ntohs(uh->uh_dport) == TFTP_SERVER &&
        ip->ip_dst.s_addr == slirp->vhost_addr.s_addr) {
        m->m_data += iphlen;
        m->m_len -= iphlen;
        tftp_input(&lhost, m);
        m->m_data -= iphlen;
        m->m_len += iphlen;
        goto bad;
    }

    if (slirp->restricted) {
        goto bad;
    }

    /*
     * Locate pcb for datagram.
     */
    so = solookup(&slirp->udp_last_so, &slirp->udb, &lhost, NULL);

    if (so == NULL) {
        /*
         * If there's no socket for this packet,
         * create one
         */
        so = socreate(slirp);
        if (udp_attach(so, AF_INET) == -1) {
            DEBUG_MISC(" udp_attach errno = %d-%s", errno, strerror(errno));
            sofree(so);
            goto bad;
        }

        /*
         * Setup fields
         */
        so->so_lfamily = AF_INET;
        so->so_laddr = ip->ip_src;
        so->so_lport = uh->uh_sport;

        if ((so->so_iptos = udp_tos(so)) == 0)
            so->so_iptos = ip->ip_tos;

        /*
         * XXXXX Here, check if it's in udpexec_list,
         * and if it is, do the fork_exec() etc.
         */
    }

    so->so_ffamily = AF_INET;
    so->so_faddr = ip->ip_dst; /* XXX */
    so->so_fport = uh->uh_dport; /* XXX */

    iphlen += sizeof(struct udphdr);
    m->m_len -= iphlen;
    m->m_data += iphlen;

    /*
     * Check for TTL
     */
    ttl = save_ip.ip_ttl-1;
    if (ttl <= 0) {
        m->m_len += iphlen;
        m->m_data -= iphlen;
        *ip = save_ip;
        DEBUG_MISC("udp ttl exceeded");
        icmp_send_error(m, ICMP_TIMXCEED, ICMP_TIMXCEED_INTRANS, 0, NULL);
        goto bad;
    }
    setsockopt(so->s, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

    /*
     * Now we sendto() the packet.
     */
    if (sosendto(so, m) == -1) {
        m->m_len += iphlen;
        m->m_data -= iphlen;
        *ip = save_ip;
        DEBUG_MISC("udp tx errno = %d-%s", errno, strerror(errno));
        icmp_send_error(m, ICMP_UNREACH, ICMP_UNREACH_NET, 0, strerror(errno));
        goto bad;
    }

    m_free(so->so_m); /* used for ICMP if error on sorecvfrom */

    /* restore the orig mbuf packet */
    m->m_len += iphlen;
    m->m_data -= iphlen;
    *ip = save_ip;
    so->so_m = m; /* ICMP backup */

    return;
bad:
    m_free(m);
}