AvahiDnsPacket *avahi_recv_dns_packet_ipv4(
        int fd,
        AvahiIPv4Address *ret_src_address,
        uint16_t *ret_src_port,
        AvahiIPv4Address *ret_dst_address,
        AvahiIfIndex *ret_iface,
        uint8_t *ret_ttl) {

    AvahiDnsPacket *p= NULL;
    struct msghdr msg;
    struct iovec io;
    size_t aux[1024 / sizeof(size_t)]; /* for alignment on ia64 ! */
    ssize_t l;
    struct cmsghdr *cmsg;
    int found_addr = 0;
    int ms;
    struct sockaddr_in sa;

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
    msg.msg_name = &sa;
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

    if (sa.sin_addr.s_addr == INADDR_ANY) {
        /* Linux 2.4 behaves very strangely sometimes! */
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
        *ret_src_address = a.data.ipv4;
    }

    if (ret_ttl)
        *ret_ttl = 255;

    if (ret_iface)
        *ret_iface = AVAHI_IF_UNSPEC;

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {

        if (cmsg->cmsg_level == IPPROTO_IP) {

            switch (cmsg->cmsg_type) {
#ifdef IP_RECVTTL
                case IP_RECVTTL:
#endif
                case IP_TTL:
                    if (ret_ttl)
                        *ret_ttl = (uint8_t) (*(int *) CMSG_DATA(cmsg));

                    break;

#ifdef IP_PKTINFO
                case IP_PKTINFO: {
                    struct in_pktinfo *i = (struct in_pktinfo*) CMSG_DATA(cmsg);

                    if (ret_iface)
                        *ret_iface = (int) i->ipi_ifindex;

                    if (ret_dst_address)
                        ret_dst_address->address = i->ipi_addr.s_addr;

                    found_addr = 1;

                    break;
                }
#endif

#ifdef IP_RECVIF
                case IP_RECVIF: {
                    struct sockaddr_dl *sdl = (struct sockaddr_dl *) CMSG_DATA (cmsg);

                    if (ret_iface)
#ifdef __sun
                        *ret_iface = *(uint_t*) sdl;
#else
                        *ret_iface = (int) sdl->sdl_index;
#endif

                    break;
                }
#endif

#ifdef IP_RECVDSTADDR
                case IP_RECVDSTADDR:
                    if (ret_dst_address)
                        memcpy(&ret_dst_address->address, CMSG_DATA (cmsg), 4);

                    found_addr = 1;
                    break;
#endif

                default:
                    avahi_log_warn("Unhandled cmsg_type : %d", cmsg->cmsg_type);
                    break;
            }
        }
    }

    assert(found_addr);

    return p;

fail:
    if (p)
        avahi_dns_packet_free(p);

    return NULL;
}