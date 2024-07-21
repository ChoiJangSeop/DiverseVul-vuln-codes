AvahiDnsPacket *avahi_recv_dns_packet_ipv6(
        int fd,
        AvahiIPv6Address *ret_src_address,
        uint16_t *ret_src_port,
        AvahiIPv6Address *ret_dst_address,
        AvahiIfIndex *ret_iface,
        uint8_t *ret_ttl) {

    AvahiDnsPacket *p = NULL;
    struct msghdr msg;
    struct iovec io;
    size_t aux[1024 / sizeof(size_t)];
    ssize_t l;
    int ms;
    struct cmsghdr *cmsg;
    int found_ttl = 0, found_iface = 0;
    struct sockaddr_in6 sa;

    assert(fd >= 0);

    if (ioctl(fd, FIONREAD, &ms) < 0) {
        avahi_log_warn("ioctl(): %s", strerror(errno));
        goto fail;
    }

    if (ms < 0) {
        avahi_log_warn("FIONREAD returned negative value.");
        goto fail;
    }

    /* For corrupt packets FIONREAD returns zero size (See rhbz #607297) */
    if (!ms)
        goto fail;

    p = avahi_dns_packet_new(ms + AVAHI_DNS_PACKET_EXTRA_SIZE);

    io.iov_base = AVAHI_DNS_PACKET_DATA(p);
    io.iov_len = p->max_size;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (struct sockaddr*) &sa;
    msg.msg_namelen = sizeof(sa);

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = aux;
    msg.msg_controllen = sizeof(aux);
    msg.msg_flags = 0;

    if ((l = recvmsg(fd, &msg, 0)) < 0) {
        /* Linux returns EAGAIN when an invalid IP packet has been
        received. We suppress warnings in this case because this might
        create quite a bit of log traffic on machines with unstable
        links. (See #60) */

        if (errno != EAGAIN)
            avahi_log_warn("recvmsg(): %s", strerror(errno));

        goto fail;
    }

    assert(!(msg.msg_flags & MSG_CTRUNC));
    assert(!(msg.msg_flags & MSG_TRUNC));

    p->size = (size_t) l;

    if (ret_src_port)
        *ret_src_port = avahi_port_from_sockaddr((struct sockaddr*) &sa);

    if (ret_src_address) {
        AvahiAddress a;
        avahi_address_from_sockaddr((struct sockaddr*) &sa, &a);
        *ret_src_address = a.data.ipv6;
    }

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {

        if (cmsg->cmsg_level == IPPROTO_IPV6) {

            switch (cmsg->cmsg_type) {

                case IPV6_HOPLIMIT:

                    if (ret_ttl)
                        *ret_ttl = (uint8_t) (*(int *) CMSG_DATA(cmsg));

                    found_ttl = 1;

                    break;

                case IPV6_PKTINFO: {
                    struct in6_pktinfo *i = (struct in6_pktinfo*) CMSG_DATA(cmsg);

                    if (ret_iface)
                        *ret_iface = i->ipi6_ifindex;

                    if (ret_dst_address)
                        memcpy(ret_dst_address->address, i->ipi6_addr.s6_addr, 16);

                    found_iface = 1;
                    break;
                }

                default:
                    avahi_log_warn("Unhandled cmsg_type : %d", cmsg->cmsg_type);
                    break;
            }
        }
    }

    assert(found_iface);
    assert(found_ttl);

    return p;

fail:
    if (p)
        avahi_dns_packet_free(p);

    return NULL;
}