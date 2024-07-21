static int _lldp_send(struct lldpd *global,
    struct lldpd_hardware *hardware,
    u_int8_t c_id_subtype,
    char *c_id,
    int c_id_len,
    u_int8_t p_id_subtype,
    char *p_id,
    int p_id_len,
    int shutdown)
{
	struct lldpd_port *port;
	struct lldpd_chassis *chassis;
	struct lldpd_frame *frame;
	int length;
	u_int8_t *packet, *pos, *tlv;
	struct lldpd_mgmt *mgmt;
	int proto;

	u_int8_t mcastaddr[] = LLDP_MULTICAST_ADDR;
#ifdef ENABLE_DOT1
	const u_int8_t dot1[] = LLDP_TLV_ORG_DOT1;
	struct lldpd_vlan *vlan;
	struct lldpd_ppvid *ppvid;
	struct lldpd_pi *pi;
#endif
#ifdef ENABLE_DOT3
	const u_int8_t dot3[] = LLDP_TLV_ORG_DOT3;
#endif
#ifdef ENABLE_LLDPMED
	int i;
	const u_int8_t med[] = LLDP_TLV_ORG_MED;
#endif
#ifdef ENABLE_CUSTOM
	struct lldpd_custom *custom;
#endif
	port = &hardware->h_lport;
	chassis = port->p_chassis;
	length = hardware->h_mtu;
	if ((packet = (u_int8_t*)calloc(1, length)) == NULL)
		return ENOMEM;
	pos = packet;

	/* Ethernet header */
	if (!(
	      /* LLDP multicast address */
	      POKE_BYTES(mcastaddr, sizeof(mcastaddr)) &&
	      /* Source MAC address */
	      POKE_BYTES(&hardware->h_lladdr, ETHER_ADDR_LEN) &&
	      /* LLDP frame */
	      POKE_UINT16(ETHERTYPE_LLDP)))
		goto toobig;

	/* Chassis ID */
	if (!(
	      POKE_START_LLDP_TLV(LLDP_TLV_CHASSIS_ID) &&
	      POKE_UINT8(c_id_subtype) &&
	      POKE_BYTES(c_id, c_id_len) &&
	      POKE_END_LLDP_TLV))
		goto toobig;

	/* Port ID */
	if (!(
	      POKE_START_LLDP_TLV(LLDP_TLV_PORT_ID) &&
	      POKE_UINT8(p_id_subtype) &&
	      POKE_BYTES(p_id, p_id_len) &&
	      POKE_END_LLDP_TLV))
		goto toobig;

	/* Time to live */
	if (!(
	      POKE_START_LLDP_TLV(LLDP_TLV_TTL) &&
	      POKE_UINT16(shutdown?0:chassis->c_ttl) &&
	      POKE_END_LLDP_TLV))
		goto toobig;

	if (shutdown)
		goto end;

	/* System name */
	if (chassis->c_name && *chassis->c_name != '\0') {
		if (!(
			    POKE_START_LLDP_TLV(LLDP_TLV_SYSTEM_NAME) &&
			    POKE_BYTES(chassis->c_name, strlen(chassis->c_name)) &&
			    POKE_END_LLDP_TLV))
			goto toobig;
	}

	/* System description (skip it if empty) */
	if (chassis->c_descr && *chassis->c_descr != '\0') {
		if (!(
			    POKE_START_LLDP_TLV(LLDP_TLV_SYSTEM_DESCR) &&
			    POKE_BYTES(chassis->c_descr, strlen(chassis->c_descr)) &&
			    POKE_END_LLDP_TLV))
			goto toobig;
	}

	/* System capabilities */
	if (global->g_config.c_cap_advertise && chassis->c_cap_available) {
		if (!(
			    POKE_START_LLDP_TLV(LLDP_TLV_SYSTEM_CAP) &&
			    POKE_UINT16(chassis->c_cap_available) &&
			    POKE_UINT16(chassis->c_cap_enabled) &&
			    POKE_END_LLDP_TLV))
			goto toobig;
	}

	/* Management addresses */
	TAILQ_FOREACH(mgmt, &chassis->c_mgmt, m_entries) {
		proto = lldpd_af_to_lldp_proto(mgmt->m_family);
		assert(proto != LLDP_MGMT_ADDR_NONE);
		if (!(
			  POKE_START_LLDP_TLV(LLDP_TLV_MGMT_ADDR) &&
			  /* Size of the address, including its type */
			  POKE_UINT8(mgmt->m_addrsize + 1) &&
			  POKE_UINT8(proto) &&
			  POKE_BYTES(&mgmt->m_addr, mgmt->m_addrsize)))
			goto toobig;

		/* Interface port type, OID */
		if (mgmt->m_iface == 0) {
			if (!(
				  /* We don't know the management interface */
				  POKE_UINT8(LLDP_MGMT_IFACE_UNKNOWN) &&
				  POKE_UINT32(0)))
				goto toobig;
		} else {
			if (!(
				  /* We have the index of the management interface */
				  POKE_UINT8(LLDP_MGMT_IFACE_IFINDEX) &&
				  POKE_UINT32(mgmt->m_iface)))
				goto toobig;
		}
		if (!(
			  /* We don't provide an OID for management */
			  POKE_UINT8(0) &&
			  POKE_END_LLDP_TLV))
			goto toobig;
	}

	/* Port description */
	if (port->p_descr && *port->p_descr != '\0') {
		if (!(
			    POKE_START_LLDP_TLV(LLDP_TLV_PORT_DESCR) &&
			    POKE_BYTES(port->p_descr, strlen(port->p_descr)) &&
			    POKE_END_LLDP_TLV))
			goto toobig;
	}

#ifdef ENABLE_DOT1
	/* Port VLAN ID */
	if(port->p_pvid != 0) {
		if (!(
		    POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
		    POKE_BYTES(dot1, sizeof(dot1)) &&
		    POKE_UINT8(LLDP_TLV_DOT1_PVID) &&
		    POKE_UINT16(port->p_pvid) &&
		    POKE_END_LLDP_TLV)) {
		    goto toobig;
		}
	}
	/* Port and Protocol VLAN IDs */
	TAILQ_FOREACH(ppvid, &port->p_ppvids, p_entries) {
		if (!(
		      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
		      POKE_BYTES(dot1, sizeof(dot1)) &&
		      POKE_UINT8(LLDP_TLV_DOT1_PPVID) &&
		      POKE_UINT8(ppvid->p_cap_status) &&
		      POKE_UINT16(ppvid->p_ppvid) &&
		      POKE_END_LLDP_TLV)) {
			goto toobig;
		}
	}
	/* VLANs */
	TAILQ_FOREACH(vlan, &port->p_vlans, v_entries) {
		if (!(
		      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
		      POKE_BYTES(dot1, sizeof(dot1)) &&
		      POKE_UINT8(LLDP_TLV_DOT1_VLANNAME) &&
		      POKE_UINT16(vlan->v_vid) &&
		      POKE_UINT8(strlen(vlan->v_name)) &&
		      POKE_BYTES(vlan->v_name, strlen(vlan->v_name)) &&
		      POKE_END_LLDP_TLV))
			goto toobig;
	}
	/* Protocol Identities */
	TAILQ_FOREACH(pi, &port->p_pids, p_entries) {
		if (!(
		      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
		      POKE_BYTES(dot1, sizeof(dot1)) &&
		      POKE_UINT8(LLDP_TLV_DOT1_PI) &&
		      POKE_UINT8(pi->p_pi_len) &&
		      POKE_BYTES(pi->p_pi, pi->p_pi_len) &&
		      POKE_END_LLDP_TLV))
			goto toobig;
	}
#endif

#ifdef ENABLE_DOT3
	/* Aggregation status */
	if (!(
	      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
	      POKE_BYTES(dot3, sizeof(dot3)) &&
	      POKE_UINT8(LLDP_TLV_DOT3_LA) &&
	      /* Bit 0 = capability ; Bit 1 = status */
	      POKE_UINT8((port->p_aggregid) ? 3:1) &&
	      POKE_UINT32(port->p_aggregid) &&
	      POKE_END_LLDP_TLV))
		goto toobig;

	/* MAC/PHY */
	if (!(
	      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
	      POKE_BYTES(dot3, sizeof(dot3)) &&
	      POKE_UINT8(LLDP_TLV_DOT3_MAC) &&
	      POKE_UINT8(port->p_macphy.autoneg_support |
			 (port->p_macphy.autoneg_enabled << 1)) &&
	      POKE_UINT16(port->p_macphy.autoneg_advertised) &&
	      POKE_UINT16(port->p_macphy.mau_type) &&
	      POKE_END_LLDP_TLV))
		goto toobig;

	/* MFS */
	if (port->p_mfs) {
		if (!(
		      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
		      POKE_BYTES(dot3, sizeof(dot3)) &&
		      POKE_UINT8(LLDP_TLV_DOT3_MFS) &&
		      POKE_UINT16(port->p_mfs) &&
		      POKE_END_LLDP_TLV))
			goto toobig;
	}
	/* Power */
	if (port->p_power.devicetype) {
		if (!(
		      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
		      POKE_BYTES(dot3, sizeof(dot3)) &&
		      POKE_UINT8(LLDP_TLV_DOT3_POWER) &&
		      POKE_UINT8((
				  (((2 - port->p_power.devicetype)    %(1<< 1))<<0) |
				  (( port->p_power.supported          %(1<< 1))<<1) |
				  (( port->p_power.enabled            %(1<< 1))<<2) |
				  (( port->p_power.paircontrol        %(1<< 1))<<3))) &&
		      POKE_UINT8(port->p_power.pairs) &&
		      POKE_UINT8(port->p_power.class)))
			goto toobig;
		/* 802.3at */
		if (port->p_power.powertype != LLDP_DOT3_POWER_8023AT_OFF) {
			if (!(
			      POKE_UINT8((
					  (((port->p_power.powertype ==
					      LLDP_DOT3_POWER_8023AT_TYPE1)?1:0) << 7) |
					   (((port->p_power.devicetype ==
					      LLDP_DOT3_POWER_PSE)?0:1) << 6) |
					   ((port->p_power.source   %(1<< 2))<<4) |
					   ((port->p_power.priority %(1<< 2))<<0))) &&
			      POKE_UINT16(port->p_power.requested) &&
			      POKE_UINT16(port->p_power.allocated)))
				goto toobig;
		}
		if (!(POKE_END_LLDP_TLV))
			goto toobig;
	}
#endif

#ifdef ENABLE_LLDPMED
	if (port->p_med_cap_enabled) {
		/* LLDP-MED cap */
		if (!(
		      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
		      POKE_BYTES(med, sizeof(med)) &&
		      POKE_UINT8(LLDP_TLV_MED_CAP) &&
		      POKE_UINT16(chassis->c_med_cap_available) &&
		      POKE_UINT8(chassis->c_med_type) &&
		      POKE_END_LLDP_TLV))
			goto toobig;

		/* LLDP-MED inventory */
#define LLDP_INVENTORY(value, subtype)					\
		if (value) {						\
		    if (!(						\
			  POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&		\
			  POKE_BYTES(med, sizeof(med)) &&		\
			  POKE_UINT8(subtype) &&			\
			  POKE_BYTES(value,				\
				(strlen(value)>32)?32:strlen(value)) &&	\
			  POKE_END_LLDP_TLV))				\
			    goto toobig;				\
		}

		if (port->p_med_cap_enabled & LLDP_MED_CAP_IV) {
			LLDP_INVENTORY(chassis->c_med_hw,
			    LLDP_TLV_MED_IV_HW);
			LLDP_INVENTORY(chassis->c_med_fw,
			    LLDP_TLV_MED_IV_FW);
			LLDP_INVENTORY(chassis->c_med_sw,
			    LLDP_TLV_MED_IV_SW);
			LLDP_INVENTORY(chassis->c_med_sn,
			    LLDP_TLV_MED_IV_SN);
			LLDP_INVENTORY(chassis->c_med_manuf,
			    LLDP_TLV_MED_IV_MANUF);
			LLDP_INVENTORY(chassis->c_med_model,
			    LLDP_TLV_MED_IV_MODEL);
			LLDP_INVENTORY(chassis->c_med_asset,
			    LLDP_TLV_MED_IV_ASSET);
		}

		/* LLDP-MED location */
		for (i = 0; i < LLDP_MED_LOCFORMAT_LAST; i++) {
			if (port->p_med_location[i].format == i + 1) {
				if (!(
				      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
				      POKE_BYTES(med, sizeof(med)) &&
				      POKE_UINT8(LLDP_TLV_MED_LOCATION) &&
				      POKE_UINT8(port->p_med_location[i].format) &&
				      POKE_BYTES(port->p_med_location[i].data,
					  port->p_med_location[i].data_len) &&
				      POKE_END_LLDP_TLV))
					goto toobig;
			}
		}

		/* LLDP-MED network policy */
		for (i = 0; i < LLDP_MED_APPTYPE_LAST; i++) {
			if (port->p_med_policy[i].type == i + 1) {
				if (!(
				      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
				      POKE_BYTES(med, sizeof(med)) &&
				      POKE_UINT8(LLDP_TLV_MED_POLICY) &&
				      POKE_UINT32((
					((port->p_med_policy[i].type     %(1<< 8))<<24) |
					((port->p_med_policy[i].unknown  %(1<< 1))<<23) |
					((port->p_med_policy[i].tagged   %(1<< 1))<<22) |
				      /*((0                              %(1<< 1))<<21) |*/
					((port->p_med_policy[i].vid      %(1<<12))<< 9) |
					((port->p_med_policy[i].priority %(1<< 3))<< 6) |
					((port->p_med_policy[i].dscp     %(1<< 6))<< 0) )) &&
				      POKE_END_LLDP_TLV))
					goto toobig;
			}
		}

		/* LLDP-MED POE-MDI */
		if ((port->p_med_power.devicetype == LLDP_MED_POW_TYPE_PSE) ||
		    (port->p_med_power.devicetype == LLDP_MED_POW_TYPE_PD)) {
			int devicetype = 0, source = 0;
			if (!(
			      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
			      POKE_BYTES(med, sizeof(med)) &&
			      POKE_UINT8(LLDP_TLV_MED_MDI)))
				goto toobig;
			switch (port->p_med_power.devicetype) {
			case LLDP_MED_POW_TYPE_PSE:
				devicetype = 0;
				switch (port->p_med_power.source) {
				case LLDP_MED_POW_SOURCE_PRIMARY: source = 1; break;
				case LLDP_MED_POW_SOURCE_BACKUP: source = 2; break;
				case LLDP_MED_POW_SOURCE_RESERVED: source = 3; break;
				default: source = 0; break;
				}
				break;
			case LLDP_MED_POW_TYPE_PD:
				devicetype = 1;
				switch (port->p_med_power.source) {
				case LLDP_MED_POW_SOURCE_PSE: source = 1; break;
				case LLDP_MED_POW_SOURCE_LOCAL: source = 2; break;
				case LLDP_MED_POW_SOURCE_BOTH: source = 3; break;
				default: source = 0; break;
				}
				break;
			}
			if (!(
			      POKE_UINT8((
				((devicetype                   %(1<< 2))<<6) |
				((source                       %(1<< 2))<<4) |
				((port->p_med_power.priority   %(1<< 4))<<0) )) &&
			      POKE_UINT16(port->p_med_power.val) &&
			      POKE_END_LLDP_TLV))
				goto toobig;
		}
	}
#endif

#ifdef ENABLE_CUSTOM
	TAILQ_FOREACH(custom, &port->p_custom_list, next) {
		if (!(
		      POKE_START_LLDP_TLV(LLDP_TLV_ORG) &&
		      POKE_BYTES(custom->oui, sizeof(custom->oui)) &&
		      POKE_UINT8(custom->subtype) &&
		      POKE_BYTES(custom->oui_info, custom->oui_info_len) &&
		      POKE_END_LLDP_TLV))
			goto toobig;
	}
#endif

end:
	/* END */
	if (!(
	      POKE_START_LLDP_TLV(LLDP_TLV_END) &&
	      POKE_END_LLDP_TLV))
		goto toobig;

	if (interfaces_send_helper(global, hardware,
		(char *)packet, pos - packet) == -1) {
		log_warn("lldp", "unable to send packet on real device for %s",
		    hardware->h_ifname);
		free(packet);
		return ENETDOWN;
	}

	hardware->h_tx_cnt++;

	/* We assume that LLDP frame is the reference */
	if (!shutdown && (frame = (struct lldpd_frame*)malloc(
			sizeof(int) + pos - packet)) != NULL) {
		frame->size = pos - packet;
		memcpy(&frame->frame, packet, frame->size);
		if ((hardware->h_lport.p_lastframe == NULL) ||
		    (hardware->h_lport.p_lastframe->size != frame->size) ||
		    (memcmp(hardware->h_lport.p_lastframe->frame, frame->frame,
			frame->size) != 0)) {
			free(hardware->h_lport.p_lastframe);
			hardware->h_lport.p_lastframe = frame;
			hardware->h_lport.p_lastchange = time(NULL);
		} else free(frame);
	}

	free(packet);
	return 0;

toobig:
	free(packet);
	return E2BIG;
}