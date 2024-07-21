int main(int argc, char **argv)
{
    struct ifreq ifr;
#ifndef SIOCBRADDIF
    unsigned long ifargs[4];
#endif
    int ifindex;
    int fd = -1, ctlfd = -1, unixfd = -1;
    int use_vnet = 0;
    int mtu;
    const char *bridge = NULL;
    char iface[IFNAMSIZ];
    int index;
    ACLRule *acl_rule;
    ACLList acl_list;
    int access_allowed, access_denied;
    int ret = EXIT_SUCCESS;

#ifdef CONFIG_LIBCAP
    /* if we're run from an suid binary, immediately drop privileges preserving
     * cap_net_admin */
    if (geteuid() == 0 && getuid() != geteuid()) {
        if (drop_privileges() == -1) {
            fprintf(stderr, "failed to drop privileges\n");
            return 1;
        }
    }
#endif

    /* parse arguments */
    for (index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--use-vnet") == 0) {
            use_vnet = 1;
        } else if (strncmp(argv[index], "--br=", 5) == 0) {
            bridge = &argv[index][5];
        } else if (strncmp(argv[index], "--fd=", 5) == 0) {
            unixfd = atoi(&argv[index][5]);
        } else {
            usage();
            return EXIT_FAILURE;
        }
    }

    if (bridge == NULL || unixfd == -1) {
        usage();
        return EXIT_FAILURE;
    }

    /* parse default acl file */
    QSIMPLEQ_INIT(&acl_list);
    if (parse_acl_file(DEFAULT_ACL_FILE, &acl_list) == -1) {
        fprintf(stderr, "failed to parse default acl file `%s'\n",
                DEFAULT_ACL_FILE);
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* validate bridge against acl -- default policy is to deny
     * according acl policy if we have a deny and allow both
     * then deny should always win over allow
     */
    access_allowed = 0;
    access_denied = 0;
    QSIMPLEQ_FOREACH(acl_rule, &acl_list, entry) {
        switch (acl_rule->type) {
        case ACL_ALLOW_ALL:
            access_allowed = 1;
            break;
        case ACL_ALLOW:
            if (strcmp(bridge, acl_rule->iface) == 0) {
                access_allowed = 1;
            }
            break;
        case ACL_DENY_ALL:
            access_denied = 1;
            break;
        case ACL_DENY:
            if (strcmp(bridge, acl_rule->iface) == 0) {
                access_denied = 1;
            }
            break;
        }
    }

    if ((access_allowed == 0) || (access_denied == 1)) {
        fprintf(stderr, "access denied by acl file\n");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* open a socket to use to control the network interfaces */
    ctlfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctlfd == -1) {
        fprintf(stderr, "failed to open control socket: %s\n", strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* open the tap device */
    fd = open("/dev/net/tun", O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "failed to open /dev/net/tun: %s\n", strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* request a tap device, disable PI, and add vnet header support if
     * requested and it's available. */
    prep_ifreq(&ifr, "tap%d");
    ifr.ifr_flags = IFF_TAP|IFF_NO_PI;
    if (use_vnet && has_vnet_hdr(fd)) {
        ifr.ifr_flags |= IFF_VNET_HDR;
    }

    if (ioctl(fd, TUNSETIFF, &ifr) == -1) {
        fprintf(stderr, "failed to create tun device: %s\n", strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* save tap device name */
    snprintf(iface, sizeof(iface), "%s", ifr.ifr_name);

    /* get the mtu of the bridge */
    prep_ifreq(&ifr, bridge);
    if (ioctl(ctlfd, SIOCGIFMTU, &ifr) == -1) {
        fprintf(stderr, "failed to get mtu of bridge `%s': %s\n",
                bridge, strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* save mtu */
    mtu = ifr.ifr_mtu;

    /* set the mtu of the interface based on the bridge */
    prep_ifreq(&ifr, iface);
    ifr.ifr_mtu = mtu;
    if (ioctl(ctlfd, SIOCSIFMTU, &ifr) == -1) {
        fprintf(stderr, "failed to set mtu of device `%s' to %d: %s\n",
                iface, mtu, strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* Linux uses the lowest enslaved MAC address as the MAC address of
     * the bridge.  Set MAC address to a high value so that it doesn't
     * affect the MAC address of the bridge.
     */
    if (ioctl(ctlfd, SIOCGIFHWADDR, &ifr) < 0) {
        fprintf(stderr, "failed to get MAC address of device `%s': %s\n",
                iface, strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    ifr.ifr_hwaddr.sa_data[0] = 0xFE;
    if (ioctl(ctlfd, SIOCSIFHWADDR, &ifr) < 0) {
        fprintf(stderr, "failed to set MAC address of device `%s': %s\n",
                iface, strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* add the interface to the bridge */
    prep_ifreq(&ifr, bridge);
    ifindex = if_nametoindex(iface);
#ifndef SIOCBRADDIF
    ifargs[0] = BRCTL_ADD_IF;
    ifargs[1] = ifindex;
    ifargs[2] = 0;
    ifargs[3] = 0;
    ifr.ifr_data = (void *)ifargs;
    ret = ioctl(ctlfd, SIOCDEVPRIVATE, &ifr);
#else
    ifr.ifr_ifindex = ifindex;
    ret = ioctl(ctlfd, SIOCBRADDIF, &ifr);
#endif
    if (ret == -1) {
        fprintf(stderr, "failed to add interface `%s' to bridge `%s': %s\n",
                iface, bridge, strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* bring the interface up */
    prep_ifreq(&ifr, iface);
    if (ioctl(ctlfd, SIOCGIFFLAGS, &ifr) == -1) {
        fprintf(stderr, "failed to get interface flags for `%s': %s\n",
                iface, strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    ifr.ifr_flags |= IFF_UP;
    if (ioctl(ctlfd, SIOCSIFFLAGS, &ifr) == -1) {
        fprintf(stderr, "failed to bring up interface `%s': %s\n",
                iface, strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* write fd to the domain socket */
    if (send_fd(unixfd, fd) == -1) {
        fprintf(stderr, "failed to write fd to unix socket: %s\n",
                strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* ... */

    /* profit! */

cleanup:
    if (fd >= 0) {
        close(fd);
    }
    if (ctlfd >= 0) {
        close(ctlfd);
    }
    while ((acl_rule = QSIMPLEQ_FIRST(&acl_list)) != NULL) {
        QSIMPLEQ_REMOVE_HEAD(&acl_list, entry);
        g_free(acl_rule);
    }

    return ret;
}