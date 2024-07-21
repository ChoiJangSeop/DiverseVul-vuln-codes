static void tftp_udp_output(struct tftp_session *spt, struct mbuf *m,
                            struct tftp_t *recv_tp)
{
    if (spt->client_addr.ss_family == AF_INET6) {
        struct sockaddr_in6 sa6, da6;

        sa6.sin6_addr = spt->slirp->vhost_addr6;
        sa6.sin6_port = recv_tp->udp.uh_dport;
        da6.sin6_addr = ((struct sockaddr_in6 *)&spt->client_addr)->sin6_addr;
        da6.sin6_port = spt->client_port;

        udp6_output(NULL, m, &sa6, &da6);
    } else {
        struct sockaddr_in sa4, da4;

        sa4.sin_addr = spt->slirp->vhost_addr;
        sa4.sin_port = recv_tp->udp.uh_dport;
        da4.sin_addr = ((struct sockaddr_in *)&spt->client_addr)->sin_addr;
        da4.sin_port = spt->client_port;

        udp_output(NULL, m, &sa4, &da4, IPTOS_LOWDELAY);
    }
}