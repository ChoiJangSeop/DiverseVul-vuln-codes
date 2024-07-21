sonmp_decode(struct lldpd *cfg, char *frame, int s,
    struct lldpd_hardware *hardware,
    struct lldpd_chassis **newchassis, struct lldpd_port **newport)
{
	const u_int8_t mcastaddr[] = SONMP_MULTICAST_ADDR;
	struct lldpd_chassis *chassis;
	struct lldpd_port *port;
	struct lldpd_mgmt *mgmt;
	int length, i;
	u_int8_t *pos;
	u_int8_t seg[3], rchassis;
	struct in_addr address;

	log_debug("sonmp", "decode SONMP PDU from %s",
	    hardware->h_ifname);

	if ((chassis = calloc(1, sizeof(struct lldpd_chassis))) == NULL) {
		log_warn("sonmp", "failed to allocate remote chassis");
		return -1;
	}
	TAILQ_INIT(&chassis->c_mgmt);
	if ((port = calloc(1, sizeof(struct lldpd_port))) == NULL) {
		log_warn("sonmp", "failed to allocate remote port");
		free(chassis);
		return -1;
	}
#ifdef ENABLE_DOT1
	TAILQ_INIT(&port->p_vlans);
#endif

	length = s;
	pos = (u_int8_t*)frame;
	if (length < SONMP_SIZE) {
		log_warnx("sonmp", "too short SONMP frame received on %s", hardware->h_ifname);
		goto malformed;
	}
	if (PEEK_CMP(mcastaddr, sizeof(mcastaddr)) != 0)
		/* There is two multicast address. We just handle only one of
		 * them. */
		goto malformed;
	/* We skip to LLC PID */
	PEEK_DISCARD(ETHER_ADDR_LEN); PEEK_DISCARD_UINT16;
	PEEK_DISCARD(6);
	if (PEEK_UINT16 != LLC_PID_SONMP_HELLO) {
		log_debug("sonmp", "incorrect LLC protocol ID received for SONMP on %s",
		    hardware->h_ifname);
		goto malformed;
	}

	chassis->c_id_subtype = LLDP_CHASSISID_SUBTYPE_ADDR;
	if ((chassis->c_id = calloc(1, sizeof(struct in_addr) + 1)) == NULL) {
		log_warn("sonmp", "unable to allocate memory for chassis id on %s",
			hardware->h_ifname);
		goto malformed;
	}
	chassis->c_id_len = sizeof(struct in_addr) + 1;
	chassis->c_id[0] = 1;
	PEEK_BYTES(&address, sizeof(struct in_addr));
	memcpy(chassis->c_id + 1, &address, sizeof(struct in_addr));
	if (asprintf(&chassis->c_name, "%s", inet_ntoa(address)) == -1) {
		log_warnx("sonmp", "unable to write chassis name for %s",
		    hardware->h_ifname);
		goto malformed;
	}
	PEEK_BYTES(seg, sizeof(seg));
	rchassis = PEEK_UINT8;
	for (i=0; sonmp_chassis_types[i].type != 0; i++) {
		if (sonmp_chassis_types[i].type == rchassis)
			break;
	}
	if (asprintf(&chassis->c_descr, "%s",
		sonmp_chassis_types[i].description) == -1) {
		log_warnx("sonmp", "unable to write chassis description for %s",
		    hardware->h_ifname);
		goto malformed;
	}
	mgmt = lldpd_alloc_mgmt(LLDPD_AF_IPV4, &address, sizeof(struct in_addr), 0);
	if (mgmt == NULL) {
		if (errno == ENOMEM)
			log_warn("sonmp", "unable to allocate memory for management address");
		else
			log_warn("sonmp", "too large management address received on %s",
			    hardware->h_ifname);
		goto malformed;
	}
	TAILQ_INSERT_TAIL(&chassis->c_mgmt, mgmt, m_entries);
	port->p_ttl = cfg?(cfg->g_config.c_tx_interval * cfg->g_config.c_tx_hold):
	    LLDPD_TTL;
	port->p_ttl = (port->p_ttl + 999) / 1000;

	port->p_id_subtype = LLDP_PORTID_SUBTYPE_LOCAL;
	if (asprintf(&port->p_id, "%02x-%02x-%02x",
		seg[0], seg[1], seg[2]) == -1) {
		log_warn("sonmp", "unable to allocate memory for port id on %s",
		    hardware->h_ifname);
		goto malformed;
	}
	port->p_id_len = strlen(port->p_id);

	/* Port description depend on the number of segments */
	if ((seg[0] == 0) && (seg[1] == 0)) {
		if (asprintf(&port->p_descr, "port %d",
			seg[2]) == -1) {
			log_warnx("sonmp", "unable to write port description for %s",
			    hardware->h_ifname);
			goto malformed;
		}
	} else if (seg[0] == 0) {
		if (asprintf(&port->p_descr, "port %d/%d",
			seg[1], seg[2]) == -1) {
			log_warnx("sonmp", "unable to write port description for %s",
			    hardware->h_ifname);
			goto malformed;
		}
	} else {
		if (asprintf(&port->p_descr, "port %x:%x:%x",
			seg[0], seg[1], seg[2]) == -1) {
			log_warnx("sonmp", "unable to write port description for %s",
			    hardware->h_ifname);
			goto malformed;
		}
	}
	*newchassis = chassis;
	*newport = port;
	return 1;

malformed:
	lldpd_chassis_cleanup(chassis, 1);
	lldpd_port_cleanup(port, 1);
	free(port);
	return -1;
}