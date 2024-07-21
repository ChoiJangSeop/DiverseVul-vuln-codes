static void ndisc_handler(sd_ndisc *nd, int event, void *userdata) {
        Link *link = userdata;
        int r;

        assert(link);

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return;

        switch (event) {
        case SD_NDISC_EVENT_TIMEOUT:
                dhcp6_request_address(link);

                r = sd_dhcp6_client_start(link->dhcp6_client);
                if (r < 0 && r != -EBUSY)
                        log_link_warning_errno(link, r, "Starting DHCPv6 client after NDisc timeout failed: %m");
                break;
        case SD_NDISC_EVENT_STOP:
                break;
        default:
                log_link_warning(link, "IPv6 Neighbor Discovery unknown event: %d", event);
        }
}