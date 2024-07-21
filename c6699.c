static int link_configure(Link *link) {
        int r;

        assert(link);
        assert(link->network);
        assert(link->state == LINK_STATE_PENDING);

        r = link_set_bridge_fdb(link);
        if (r < 0)
                return r;

        r = link_set_ipv4_forward(link);
        if (r < 0)
                return r;

        r = link_set_ipv6_forward(link);
        if (r < 0)
                return r;

        r = link_set_ipv6_privacy_extensions(link);
        if (r < 0)
                return r;

        r = link_set_ipv6_accept_ra(link);
        if (r < 0)
                return r;

        r = link_set_ipv6_dad_transmits(link);
        if (r < 0)
                return r;

        r = link_set_ipv6_hop_limit(link);
        if (r < 0)
                return r;

        if (link_ipv4ll_enabled(link)) {
                r = ipv4ll_configure(link);
                if (r < 0)
                        return r;
        }

        if (link_dhcp4_enabled(link)) {
                r = dhcp4_configure(link);
                if (r < 0)
                        return r;
        }

        if (link_dhcp4_server_enabled(link)) {
                r = sd_dhcp_server_new(&link->dhcp_server, link->ifindex);
                if (r < 0)
                        return r;

                r = sd_dhcp_server_attach_event(link->dhcp_server, NULL, 0);
                if (r < 0)
                        return r;
        }

        if (link_dhcp6_enabled(link)) {
                r = ndisc_configure(link);
                if (r < 0)
                        return r;
        }

        if (link_lldp_enabled(link)) {
                r = sd_lldp_new(link->ifindex, link->ifname, &link->mac, &link->lldp);
                if (r < 0)
                        return r;

                r = sd_lldp_attach_event(link->lldp, NULL, 0);
                if (r < 0)
                        return r;

                r = sd_lldp_set_callback(link->lldp,
                                         lldp_handler, link);
                if (r < 0)
                        return r;
        }

        if (link_has_carrier(link)) {
                r = link_acquire_conf(link);
                if (r < 0)
                        return r;
        }

        return link_enter_join_netdev(link);
}