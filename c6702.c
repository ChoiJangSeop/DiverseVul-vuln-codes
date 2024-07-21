static bool manager_check_idle(void *userdata) {
        Manager *m = userdata;
        Link *link;
        Iterator i;

        assert(m);

        HASHMAP_FOREACH(link, m->links, i) {
                /* we are not woken on udev activity, so let's just wait for the
                 * pending udev event */
                if (link->state == LINK_STATE_PENDING)
                        return false;

                if (!link->network)
                        continue;

                /* we are not woken on netork activity, so let's stay around */
                if (link_lldp_enabled(link) ||
                    link_ipv4ll_enabled(link) ||
                    link_dhcp4_server_enabled(link) ||
                    link_dhcp4_enabled(link) ||
                    link_dhcp6_enabled(link))
                        return false;
        }

        return true;
}