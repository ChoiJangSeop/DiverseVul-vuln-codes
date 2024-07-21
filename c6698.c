static void ndisc_router_handler(sd_ndisc *nd, uint8_t flags, const struct in6_addr *gateway, unsigned lifetime, int pref, void *userdata) {
        Link *link = userdata;
        int r;

        assert(link);
        assert(link->network);

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return;

        if (flags & (ND_RA_FLAG_MANAGED | ND_RA_FLAG_OTHER)) {
                if (flags & ND_RA_FLAG_MANAGED)
                        dhcp6_request_address(link);

                r = sd_dhcp6_client_start(link->dhcp6_client);
                if (r < 0 && r != -EBUSY)
                        log_link_warning_errno(link, r, "Starting DHCPv6 client on NDisc request failed: %m");
        }
}