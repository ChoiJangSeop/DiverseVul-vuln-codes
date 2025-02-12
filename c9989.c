lldp_decode(struct lldpd *cfg, char *frame, int s,
    struct lldpd_hardware *hardware,
    struct lldpd_chassis **newchassis, struct lldpd_port **newport)
{
	struct lldpd_chassis *chassis;
	struct lldpd_port *port;
	char lldpaddr[ETHER_ADDR_LEN];
	const char dot1[] = LLDP_TLV_ORG_DOT1;
	const char dot3[] = LLDP_TLV_ORG_DOT3;
	const char med[] = LLDP_TLV_ORG_MED;
	const char dcbx[] = LLDP_TLV_ORG_DCBX;
	unsigned char orgid[3];
	int length, gotend = 0, ttl_received = 0;
	int tlv_size, tlv_type, tlv_subtype, tlv_count = 0;
	u_int8_t *pos, *tlv;
	char *b;
#ifdef ENABLE_DOT1
	struct lldpd_vlan *vlan = NULL;
	int vlan_len;
	struct lldpd_ppvid *ppvid;
	struct lldpd_pi *pi = NULL;
#endif
	struct lldpd_mgmt *mgmt;
	int af;
	u_int8_t addr_str_length, addr_str_buffer[32];
	u_int8_t addr_family, addr_length, *addr_ptr, iface_subtype;
	u_int32_t iface_number, iface;
#ifdef ENABLE_CUSTOM
	struct lldpd_custom *custom = NULL;
#endif

	log_debug("lldp", "receive LLDP PDU on %s",
	    hardware->h_ifname);

	if ((chassis = calloc(1, sizeof(struct lldpd_chassis))) == NULL) {
		log_warn("lldp", "failed to allocate remote chassis");
		return -1;
	}
	TAILQ_INIT(&chassis->c_mgmt);
	if ((port = calloc(1, sizeof(struct lldpd_port))) == NULL) {
		log_warn("lldp", "failed to allocate remote port");
		free(chassis);
		return -1;
	}
#ifdef ENABLE_DOT1
	TAILQ_INIT(&port->p_vlans);
	TAILQ_INIT(&port->p_ppvids);
	TAILQ_INIT(&port->p_pids);
#endif
#ifdef ENABLE_CUSTOM
	TAILQ_INIT(&port->p_custom_list);
#endif

	length = s;
	pos = (u_int8_t*)frame;

	if (length < 2*ETHER_ADDR_LEN + sizeof(u_int16_t)) {
		log_warnx("lldp", "too short frame received on %s", hardware->h_ifname);
		goto malformed;
	}
	PEEK_BYTES(lldpaddr, ETHER_ADDR_LEN);
	if (memcmp(lldpaddr, (const char [])LLDP_ADDR_NEAREST_BRIDGE, ETHER_ADDR_LEN) &&
	    memcmp(lldpaddr, (const char [])LLDP_ADDR_NEAREST_NONTPMR_BRIDGE, ETHER_ADDR_LEN) &&
	    memcmp(lldpaddr, (const char [])LLDP_ADDR_NEAREST_CUSTOMER_BRIDGE, ETHER_ADDR_LEN)) {
		log_info("lldp", "frame not targeted at LLDP multicast address received on %s",
		    hardware->h_ifname);
		goto malformed;
	}
	PEEK_DISCARD(ETHER_ADDR_LEN);	/* Skip source address */
	if (PEEK_UINT16 != ETHERTYPE_LLDP) {
		log_info("lldp", "non LLDP frame received on %s",
		    hardware->h_ifname);
		goto malformed;
	}

	while (length && (!gotend)) {
		if (length < 2) {
			log_warnx("lldp", "tlv header too short received on %s",
			    hardware->h_ifname);
			goto malformed;
		}
		tlv_size = PEEK_UINT16;
		tlv_type = tlv_size >> 9;
		tlv_size = tlv_size & 0x1ff;
		(void)PEEK_SAVE(tlv);
		if (length < tlv_size) {
			log_warnx("lldp", "frame too short for tlv received on %s",
			    hardware->h_ifname);
			goto malformed;
		}
		/* Check order for mandatory TLVs */
		tlv_count++;
		switch (tlv_type) {
		case LLDP_TLV_CHASSIS_ID:
			if (tlv_count != 1) {
				log_warnx("lldp", "first TLV should be a chassis ID on %s, not %d",
				    hardware->h_ifname, tlv_type);
				goto malformed;
			}
			break;
		case LLDP_TLV_PORT_ID:
			if (tlv_count != 2) {
				log_warnx("lldp", "second TLV should be a port ID on %s, not %d",
				    hardware->h_ifname, tlv_type);
				goto malformed;
			}
			break;
		case LLDP_TLV_TTL:
			if (tlv_count != 3) {
				log_warnx("lldp", "third TLV should be a TTL on %s, not %d",
				    hardware->h_ifname, tlv_type);
				goto malformed;
			}
			break;
		}

		switch (tlv_type) {
		case LLDP_TLV_END:
			if (tlv_size != 0) {
				log_warnx("lldp", "lldp end received with size not null on %s",
				    hardware->h_ifname);
				goto malformed;
			}
			if (length)
				log_debug("lldp", "extra data after lldp end on %s",
				    hardware->h_ifname);
			gotend = 1;
			break;
		case LLDP_TLV_CHASSIS_ID:
		case LLDP_TLV_PORT_ID:
			CHECK_TLV_SIZE(2, "Port/Chassis Id");
			CHECK_TLV_MAX_SIZE(256, "Port/Chassis Id");
			tlv_subtype = PEEK_UINT8;
			if ((tlv_subtype == 0) || (tlv_subtype > 7)) {
				log_warnx("lldp", "unknown subtype for tlv id received on %s",
				    hardware->h_ifname);
				goto malformed;
			}
			if ((b = (char *)calloc(1, tlv_size - 1)) == NULL) {
				log_warn("lldp", "unable to allocate memory for id tlv "
				    "received on %s",
				    hardware->h_ifname);
				goto malformed;
			}
			PEEK_BYTES(b, tlv_size - 1);
			if (tlv_type == LLDP_TLV_PORT_ID) {
				if (port->p_id != NULL) {
					log_warnx("lldp", "Port ID TLV received twice on %s",
					    hardware->h_ifname);
					free(b);
					goto malformed;
				}
				port->p_id_subtype = tlv_subtype;
				port->p_id = b;
				port->p_id_len = tlv_size - 1;
			} else {
				if (chassis->c_id != NULL) {
					log_warnx("lldp", "Chassis ID TLV received twice on %s",
					    hardware->h_ifname);
					free(b);
					goto malformed;
				}
				chassis->c_id_subtype = tlv_subtype;
				chassis->c_id = b;
				chassis->c_id_len = tlv_size - 1;
			}
			break;
		case LLDP_TLV_TTL:
			if (ttl_received) {
				log_warnx("lldp", "TTL TLV received twice on %s",
				    hardware->h_ifname);
				goto malformed;
			}
			CHECK_TLV_SIZE(2, "TTL");
			port->p_ttl = PEEK_UINT16;
			ttl_received = 1;
			break;
		case LLDP_TLV_PORT_DESCR:
		case LLDP_TLV_SYSTEM_NAME:
		case LLDP_TLV_SYSTEM_DESCR:
			if (tlv_size < 1) {
				log_debug("lldp", "empty tlv received on %s",
				    hardware->h_ifname);
				break;
			}
			if ((b = (char *)calloc(1, tlv_size + 1)) == NULL) {
				log_warn("lldp", "unable to allocate memory for string tlv "
				    "received on %s",
				    hardware->h_ifname);
				goto malformed;
			}
			PEEK_BYTES(b, tlv_size);
			if (tlv_type == LLDP_TLV_PORT_DESCR)
				port->p_descr = b;
			else if (tlv_type == LLDP_TLV_SYSTEM_NAME)
				chassis->c_name = b;
			else chassis->c_descr = b;
			break;
		case LLDP_TLV_SYSTEM_CAP:
			CHECK_TLV_SIZE(4, "System capabilities");
			chassis->c_cap_available = PEEK_UINT16;
			chassis->c_cap_enabled = PEEK_UINT16;
			break;
		case LLDP_TLV_MGMT_ADDR:
			CHECK_TLV_SIZE(1, "Management address");
			addr_str_length = PEEK_UINT8;
			if (addr_str_length > sizeof(addr_str_buffer)) {
				log_warnx("lldp", "too large management address on %s",
				    hardware->h_ifname);
				goto malformed;
			}
			CHECK_TLV_SIZE(1 + addr_str_length, "Management address");
			PEEK_BYTES(addr_str_buffer, addr_str_length);
			addr_length = addr_str_length - 1;
			addr_family = addr_str_buffer[0];
			addr_ptr = &addr_str_buffer[1];
			CHECK_TLV_SIZE(1 + addr_str_length + 5, "Management address");
			iface_subtype = PEEK_UINT8;
			iface_number = PEEK_UINT32;

			af = lldpd_af_from_lldp_proto(addr_family);
			if (af == LLDPD_AF_UNSPEC)
				break;
			if (iface_subtype == LLDP_MGMT_IFACE_IFINDEX)
				iface = iface_number;
			else
				iface = 0;
			mgmt = lldpd_alloc_mgmt(af, addr_ptr, addr_length, iface);
			if (mgmt == NULL) {
				if (errno == ENOMEM)
					log_warn("lldp", "unable to allocate memory "
					    "for management address");
				else
					log_warn("lldp", "too large management address "
					    "received on %s", hardware->h_ifname);
				goto malformed;
			}
			TAILQ_INSERT_TAIL(&chassis->c_mgmt, mgmt, m_entries);
			break;
		case LLDP_TLV_ORG:
			CHECK_TLV_SIZE(1 + (int)sizeof(orgid), "Organisational");
			PEEK_BYTES(orgid, sizeof(orgid));
			tlv_subtype = PEEK_UINT8;
			if (memcmp(dot1, orgid, sizeof(orgid)) == 0) {
#ifndef ENABLE_DOT1
				hardware->h_rx_unrecognized_cnt++;
#else
				/* Dot1 */
				switch (tlv_subtype) {
				case LLDP_TLV_DOT1_VLANNAME:
					CHECK_TLV_SIZE(7, "VLAN");
					if ((vlan = (struct lldpd_vlan *)calloc(1,
						    sizeof(struct lldpd_vlan))) == NULL) {
						log_warn("lldp", "unable to alloc vlan "
						    "structure for "
						    "tlv received on %s",
						    hardware->h_ifname);
						goto malformed;
					}
					vlan->v_vid = PEEK_UINT16;
					vlan_len = PEEK_UINT8;
					CHECK_TLV_SIZE(7 + vlan_len, "VLAN");
					if ((vlan->v_name =
						(char *)calloc(1, vlan_len + 1)) == NULL) {
						log_warn("lldp", "unable to alloc vlan name for "
						    "tlv received on %s",
						    hardware->h_ifname);
						goto malformed;
					}
					PEEK_BYTES(vlan->v_name, vlan_len);
					TAILQ_INSERT_TAIL(&port->p_vlans,
					    vlan, v_entries);
					vlan = NULL;
					break;
				case LLDP_TLV_DOT1_PVID:
					CHECK_TLV_SIZE(6, "PVID");
					port->p_pvid = PEEK_UINT16;
					break;
				case LLDP_TLV_DOT1_PPVID:
					CHECK_TLV_SIZE(7, "PPVID");
					/* validation needed */
					/* PPVID has to be unique if more than
					   one PPVID TLVs are received  - 
					   discard if duplicate */
					/* if support bit is not set and 
					   enabled bit is set - PPVID TLV is
					   considered error  and discarded */
					/* if PPVID > 4096 - bad and discard */
					if ((ppvid = (struct lldpd_ppvid *)calloc(1,
						    sizeof(struct lldpd_ppvid))) == NULL) {
						log_warn("lldp", "unable to alloc ppvid "
						    "structure for "
						    "tlv received on %s",
						    hardware->h_ifname);
						goto malformed;
					}
					ppvid->p_cap_status = PEEK_UINT8;
					ppvid->p_ppvid = PEEK_UINT16;	
					TAILQ_INSERT_TAIL(&port->p_ppvids,
					    ppvid, p_entries);
					break;
				case LLDP_TLV_DOT1_PI:
					/* validation needed */
					/* PI has to be unique if more than 
					   one PI TLVs are received  - discard
					   if duplicate ?? */
					CHECK_TLV_SIZE(5, "PI");
					if ((pi = (struct lldpd_pi *)calloc(1,
						    sizeof(struct lldpd_pi))) == NULL) {
						log_warn("lldp", "unable to alloc PI "
						    "structure for "
						    "tlv received on %s",
						    hardware->h_ifname);
						goto malformed;
					}
					pi->p_pi_len = PEEK_UINT8;
					CHECK_TLV_SIZE(5 + pi->p_pi_len, "PI");
					if ((pi->p_pi =
						(char *)calloc(1, pi->p_pi_len)) == NULL) {
						log_warn("lldp", "unable to alloc pid name for "
						    "tlv received on %s",
						    hardware->h_ifname);
						goto malformed;
					}
					PEEK_BYTES(pi->p_pi, pi->p_pi_len);
					TAILQ_INSERT_TAIL(&port->p_pids,
					    pi, p_entries);
					pi = NULL;
					break;
				default:
					/* Unknown Dot1 TLV, ignore it */
					hardware->h_rx_unrecognized_cnt++;
				}
#endif
			} else if (memcmp(dot3, orgid, sizeof(orgid)) == 0) {
#ifndef ENABLE_DOT3
				hardware->h_rx_unrecognized_cnt++;
#else
				/* Dot3 */
				switch (tlv_subtype) {
				case LLDP_TLV_DOT3_MAC:
					CHECK_TLV_SIZE(9, "MAC/PHY");
					port->p_macphy.autoneg_support = PEEK_UINT8;
					port->p_macphy.autoneg_enabled =
					    (port->p_macphy.autoneg_support & 0x2) >> 1;
					port->p_macphy.autoneg_support =
					    port->p_macphy.autoneg_support & 0x1;
					port->p_macphy.autoneg_advertised =
					    PEEK_UINT16;
					port->p_macphy.mau_type = PEEK_UINT16;
					break;
				case LLDP_TLV_DOT3_LA:
					CHECK_TLV_SIZE(9, "Link aggregation");
					PEEK_DISCARD_UINT8;
					port->p_aggregid = PEEK_UINT32;
					break;
				case LLDP_TLV_DOT3_MFS:
					CHECK_TLV_SIZE(6, "MFS");
					port->p_mfs = PEEK_UINT16;
					break;
				case LLDP_TLV_DOT3_POWER:
					CHECK_TLV_SIZE(7, "Power");
					port->p_power.devicetype = PEEK_UINT8;
					port->p_power.supported =
						(port->p_power.devicetype & 0x2) >> 1;
					port->p_power.enabled =
						(port->p_power.devicetype & 0x4) >> 2;
					port->p_power.paircontrol =
						(port->p_power.devicetype & 0x8) >> 3;
					port->p_power.devicetype =
						(port->p_power.devicetype & 0x1)?
						LLDP_DOT3_POWER_PSE:LLDP_DOT3_POWER_PD;
					port->p_power.pairs = PEEK_UINT8;
					port->p_power.class = PEEK_UINT8;
					/* 802.3at? */
					if (tlv_size >= 12) {
						port->p_power.powertype = PEEK_UINT8;
						port->p_power.source =
						    (port->p_power.powertype & (1<<5 | 1<<4)) >> 4;
						port->p_power.priority =
						    (port->p_power.powertype & (1<<1 | 1<<0));
						port->p_power.powertype =
						    (port->p_power.powertype & (1<<7))?
						    LLDP_DOT3_POWER_8023AT_TYPE1:
						    LLDP_DOT3_POWER_8023AT_TYPE2;
						port->p_power.requested = PEEK_UINT16;
						port->p_power.allocated = PEEK_UINT16;
					} else
						port->p_power.powertype =
						    LLDP_DOT3_POWER_8023AT_OFF;
					/* 802.3bt? */
					if (tlv_size >= 29) {
						port->p_power.requested_a = PEEK_UINT16;
						port->p_power.requested_b = PEEK_UINT16;
						port->p_power.allocated_a = PEEK_UINT16;
						port->p_power.allocated_b = PEEK_UINT16;
						port->p_power.pse_status = PEEK_UINT16;
						port->p_power.pd_status =
						    (port->p_power.pse_status & (1<<13 | 1<<12)) >> 12;
						port->p_power.pse_pairs_ext =
						    (port->p_power.pse_status & (1<<11 | 1<<10)) >> 10;
						port->p_power.class_a =
						    (port->p_power.pse_status & (1<<9 | 1<<8 | 1<<7)) >> 7;
						port->p_power.class_b =
						    (port->p_power.pse_status & (1<<6 | 1<<5 | 1<<4)) >> 4;
						port->p_power.class_ext =
						    (port->p_power.pse_status & 0xf);
						port->p_power.pse_status =
						    (port->p_power.pse_status & (1<<15 | 1<<14)) >> 14;
						port->p_power.type_ext = PEEK_UINT8;
						port->p_power.pd_load =
						    (port->p_power.type_ext & 0x1);
						port->p_power.type_ext =
						    ((port->p_power.type_ext & (1<<3 | 1<<2 | 1<<1)) + 1);
						port->p_power.pse_max = PEEK_UINT16;
					} else {
						port->p_power.type_ext =
						    LLDP_DOT3_POWER_8023BT_OFF;
					}
					break;
				default:
					/* Unknown Dot3 TLV, ignore it */
					hardware->h_rx_unrecognized_cnt++;
				}
#endif
			} else if (memcmp(med, orgid, sizeof(orgid)) == 0) {
				/* LLDP-MED */
#ifndef ENABLE_LLDPMED
				hardware->h_rx_unrecognized_cnt++;
#else
				u_int32_t policy;
				unsigned loctype;
				unsigned power;

				switch (tlv_subtype) {
				case LLDP_TLV_MED_CAP:
					CHECK_TLV_SIZE(7, "LLDP-MED capabilities");
					chassis->c_med_cap_available = PEEK_UINT16;
					chassis->c_med_type = PEEK_UINT8;
					port->p_med_cap_enabled |=
					    LLDP_MED_CAP_CAP;
					break;
				case LLDP_TLV_MED_POLICY:
					CHECK_TLV_SIZE(8, "LLDP-MED policy");
					policy = PEEK_UINT32;
					if (((policy >> 24) < 1) ||
					    ((policy >> 24) > LLDP_MED_APPTYPE_LAST)) {
						log_info("lldp", "unknown policy field %d "
						    "received on %s",
						    policy,
						    hardware->h_ifname);
						break;
					}
					port->p_med_policy[(policy >> 24) - 1].type =
					    (policy >> 24);
					port->p_med_policy[(policy >> 24) - 1].unknown =
					    ((policy & 0x800000) != 0);
					port->p_med_policy[(policy >> 24) - 1].tagged =
					    ((policy & 0x400000) != 0);
					port->p_med_policy[(policy >> 24) - 1].vid =
					    (policy & 0x001FFE00) >> 9;
					port->p_med_policy[(policy >> 24) - 1].priority =
					    (policy & 0x1C0) >> 6;
					port->p_med_policy[(policy >> 24) - 1].dscp =
					    policy & 0x3F;
					port->p_med_cap_enabled |=
					    LLDP_MED_CAP_POLICY;
					break;
				case LLDP_TLV_MED_LOCATION:
					CHECK_TLV_SIZE(5, "LLDP-MED Location");
					loctype = PEEK_UINT8;
					if ((loctype < 1) ||
					    (loctype > LLDP_MED_LOCFORMAT_LAST)) {
						log_info("lldp", "unknown location type "
						    "received on %s",
						    hardware->h_ifname);
						break;
					}
					if ((port->p_med_location[loctype - 1].data =
						(char*)malloc(tlv_size - 5)) == NULL) {
						log_warn("lldp", "unable to allocate memory "
						    "for LLDP-MED location for "
						    "frame received on %s",
						    hardware->h_ifname);
						goto malformed;
					}
					PEEK_BYTES(port->p_med_location[loctype - 1].data,
					    tlv_size - 5);
					port->p_med_location[loctype - 1].data_len =
					    tlv_size - 5;
					port->p_med_location[loctype - 1].format = loctype;
					port->p_med_cap_enabled |=
					    LLDP_MED_CAP_LOCATION;
					break;
				case LLDP_TLV_MED_MDI:
					CHECK_TLV_SIZE(7, "LLDP-MED PoE-MDI");
					power = PEEK_UINT8;
					switch (power & 0xC0) {
					case 0x0:
						port->p_med_power.devicetype = LLDP_MED_POW_TYPE_PSE;
						port->p_med_cap_enabled |=
						    LLDP_MED_CAP_MDI_PSE;
						switch (power & 0x30) {
						case 0x0:
							port->p_med_power.source =
							    LLDP_MED_POW_SOURCE_UNKNOWN;
							break;
						case 0x10:
							port->p_med_power.source =
							    LLDP_MED_POW_SOURCE_PRIMARY;
							break;
						case 0x20:
							port->p_med_power.source =
							    LLDP_MED_POW_SOURCE_BACKUP;
							break;
						default:
							port->p_med_power.source =
							    LLDP_MED_POW_SOURCE_RESERVED;
						}
						break;
					case 0x40:
						port->p_med_power.devicetype = LLDP_MED_POW_TYPE_PD;
						port->p_med_cap_enabled |=
						    LLDP_MED_CAP_MDI_PD;
						switch (power & 0x30) {
						case 0x0:
							port->p_med_power.source =
							    LLDP_MED_POW_SOURCE_UNKNOWN;
							break;
						case 0x10:
							port->p_med_power.source =
							    LLDP_MED_POW_SOURCE_PSE;
							break;
						case 0x20:
							port->p_med_power.source =
							    LLDP_MED_POW_SOURCE_LOCAL;
							break;
						default:
							port->p_med_power.source =
							    LLDP_MED_POW_SOURCE_BOTH;
						}
						break;
					default:
						port->p_med_power.devicetype =
						    LLDP_MED_POW_TYPE_RESERVED;
					}
					if ((power & 0x0F) > LLDP_MED_POW_PRIO_LOW)
						port->p_med_power.priority =
						    LLDP_MED_POW_PRIO_UNKNOWN;
					else
						port->p_med_power.priority =
						    power & 0x0F;
					port->p_med_power.val = PEEK_UINT16;
					break;
				case LLDP_TLV_MED_IV_HW:
				case LLDP_TLV_MED_IV_SW:
				case LLDP_TLV_MED_IV_FW:
				case LLDP_TLV_MED_IV_SN:
				case LLDP_TLV_MED_IV_MANUF:
				case LLDP_TLV_MED_IV_MODEL:
				case LLDP_TLV_MED_IV_ASSET:
					if (tlv_size <= 4)
						b = NULL;
					else {
						if ((b = (char*)malloc(tlv_size - 3)) ==
						    NULL) {
							log_warn("lldp", "unable to allocate "
							    "memory for LLDP-MED "
							    "inventory for frame "
							    "received on %s",
							    hardware->h_ifname);
							goto malformed;
						}
						PEEK_BYTES(b, tlv_size - 4);
						b[tlv_size - 4] = '\0';
					}
					switch (tlv_subtype) {
					case LLDP_TLV_MED_IV_HW:
						chassis->c_med_hw = b;
						break;
					case LLDP_TLV_MED_IV_FW:
						chassis->c_med_fw = b;
						break;
					case LLDP_TLV_MED_IV_SW:
						chassis->c_med_sw = b;
						break;
					case LLDP_TLV_MED_IV_SN:
						chassis->c_med_sn = b;
						break;
					case LLDP_TLV_MED_IV_MANUF:
						chassis->c_med_manuf = b;
						break;
					case LLDP_TLV_MED_IV_MODEL:
						chassis->c_med_model = b;
						break;
					case LLDP_TLV_MED_IV_ASSET:
						chassis->c_med_asset = b;
						break;
					}
					port->p_med_cap_enabled |=
					    LLDP_MED_CAP_IV;
					break;
				default:
					/* Unknown LLDP MED, ignore it */
					hardware->h_rx_unrecognized_cnt++;
				}
#endif /* ENABLE_LLDPMED */
			} else if (memcmp(dcbx, orgid, sizeof(orgid)) == 0) {
				log_debug("lldp", "unsupported DCBX tlv received on %s - ignore",
				    hardware->h_ifname);
				hardware->h_rx_unrecognized_cnt++;
			} else {
				log_debug("lldp", "unknown org tlv [%02x:%02x:%02x] received on %s",
				    orgid[0], orgid[1], orgid[2],
				    hardware->h_ifname);
				hardware->h_rx_unrecognized_cnt++;
#ifdef ENABLE_CUSTOM
				custom = (struct lldpd_custom*)calloc(1, sizeof(struct lldpd_custom));
				if (!custom) {
					log_warn("lldp",
					    "unable to allocate memory for custom TLV");
					goto malformed;
				}
				custom->oui_info_len = tlv_size > 4 ? tlv_size - 4 : 0;
				memcpy(custom->oui, orgid, sizeof(custom->oui));
				custom->subtype = tlv_subtype;
				if (custom->oui_info_len > 0) {
					custom->oui_info = malloc(custom->oui_info_len);
					if (!custom->oui_info) {
						log_warn("lldp",
						    "unable to allocate memory for custom TLV data");
						goto malformed;
					}
					PEEK_BYTES(custom->oui_info, custom->oui_info_len);
				}
				TAILQ_INSERT_TAIL(&port->p_custom_list, custom, next);
				custom = NULL;
#endif
			}
			break;
		default:
			log_warnx("lldp", "unknown tlv (%d) received on %s",
			    tlv_type, hardware->h_ifname);
			hardware->h_rx_unrecognized_cnt++;
			break;
		}
		if (pos > tlv + tlv_size) {
			log_warnx("lldp", "BUG: already past TLV!");
			goto malformed;
		}
		PEEK_DISCARD(tlv + tlv_size - pos);
	}

	/* Some random check */
	if ((chassis->c_id == NULL) ||
	    (port->p_id == NULL) ||
	    (!ttl_received) ||
	    (gotend == 0)) {
		log_warnx("lldp", "some mandatory tlv are missing for frame received on %s",
		    hardware->h_ifname);
		goto malformed;
	}
	*newchassis = chassis;
	*newport = port;
	return 1;
malformed:
#ifdef ENABLE_CUSTOM
	free(custom);
#endif
#ifdef ENABLE_DOT1
	free(vlan);
	free(pi);
#endif
	lldpd_chassis_cleanup(chassis, 1);
	lldpd_port_cleanup(port, 1);
	free(port);
	return -1;
}