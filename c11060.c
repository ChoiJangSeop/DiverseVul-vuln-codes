START_TEST (test_send_sonmp)
{
	int n;
	/* Packet we should build:
IEEE 802.3 Ethernet 
    Destination: Bay-Networks-(Synoptics)-autodiscovery (01:00:81:00:01:00)
    Source: 5e:10:8e:e7:84:ad (5e:10:8e:e7:84:ad)
    Length: 22
Logical-Link Control
    DSAP: SNAP (0xaa)
    IG Bit: Individual
    SSAP: SNAP (0xaa)
    CR Bit: Command
    Control field: U, func=UI (0x03)
        000. 00.. = Command: Unnumbered Information (0x00)
        .... ..11 = Frame type: Unnumbered frame (0x03)
    Organization Code: Nortel Networks SONMP (0x000081)
    PID: SONMP segment hello (0x01a2)
Nortel Networks / SynOptics Network Management Protocol
    NMM IP address: 172.17.142.37 (172.17.142.37)
    Segment Identifier: 0x000004
    Chassis type: Unknown (1)
    Backplane type: ethernet, fast ethernet and gigabit ethernet (12)
    NMM state: New (3)
    Number of links: 1

IEEE 802.3 Ethernet 
    Destination: Bay-Networks-(Synoptics)-autodiscovery (01:00:81:00:01:01)
    Source: 5e:10:8e:e7:84:ad (5e:10:8e:e7:84:ad)
    Length: 22
Logical-Link Control
    DSAP: SNAP (0xaa)
    IG Bit: Individual
    SSAP: SNAP (0xaa)
    CR Bit: Command
    Control field: U, func=UI (0x03)
        000. 00.. = Command: Unnumbered Information (0x00)
        .... ..11 = Frame type: Unnumbered frame (0x03)
    Organization Code: Nortel Networks SONMP (0x000081)
    PID: SONMP flatnet hello (0x01a1)
Nortel Networks / SynOptics Network Management Protocol
    NMM IP address: 172.17.142.37 (172.17.142.37)
    Segment Identifier: 0x000004
    Chassis type: Unknown (1)
    Backplane type: ethernet, fast ethernet and gigabit ethernet (12)
    NMM state: New (3)
    Number of links: 1
	*/
	char pkt1[] = {
		0x01, 0x00, 0x81, 0x00, 0x01, 0x00, 0x5e, 0x10,
		0x8e, 0xe7, 0x84, 0xad, 0x00, 0x16, 0xaa, 0xaa,
		0x03, 0x00, 0x00, 0x81, 0x01, 0xa2, 0xac, 0x11,
		0x8e, 0x25, 0x00, 0x00, 0x04, 0x01, 0x0c, 0x03,
		0x01 };
	char pkt2[] = {
		0x01, 0x00, 0x81, 0x00, 0x01, 0x01, 0x5e, 0x10,
		0x8e, 0xe7, 0x84, 0xad, 0x00, 0x16, 0xaa, 0xaa,
		0x03, 0x00, 0x00, 0x81, 0x01, 0xa1, 0xac, 0x11,
		0x8e, 0x25, 0x00, 0x00, 0x04, 0x01, 0x0c, 0x03,
		0x01 };
	struct packet *pkt;
	in_addr_t addr;	
	struct lldpd_mgmt *mgmt;

	/* Populate port and chassis */
	hardware.h_lport.p_id_subtype = LLDP_PORTID_SUBTYPE_IFNAME;
	hardware.h_lport.p_id = "Not used";
	hardware.h_lport.p_id_len = strlen(hardware.h_lport.p_id);
	chassis.c_id_subtype = LLDP_CHASSISID_SUBTYPE_LLADDR;
	chassis.c_id = macaddress;
	chassis.c_id_len = ETHER_ADDR_LEN;
	TAILQ_INIT(&chassis.c_mgmt);
	addr = inet_addr("172.17.142.37");
	mgmt = lldpd_alloc_mgmt(LLDPD_AF_IPV4, 
				&addr, sizeof(in_addr_t), 0);
	if (mgmt == NULL)
		ck_abort();
	TAILQ_INSERT_TAIL(&chassis.c_mgmt, mgmt, m_entries);

	/* Build packet */
	n = sonmp_send(NULL, &hardware);
	if (n != 0) {
		fail("unable to build packet");
		return;
	}
	if (TAILQ_EMPTY(&pkts)) {
		fail("no packets sent");
		return;
	}
	pkt = TAILQ_FIRST(&pkts);
	ck_assert_int_eq(pkt->size, sizeof(pkt1));
	fail_unless(memcmp(pkt->data, pkt1, sizeof(pkt1)) == 0);
	pkt = TAILQ_NEXT(pkt, next);
	if (!pkt) {
		fail("need one more packet");
		return;
	}
	ck_assert_int_eq(pkt->size, sizeof(pkt2));
	fail_unless(memcmp(pkt->data, pkt2, sizeof(pkt2)) == 0);
	fail_unless(TAILQ_NEXT(pkt, next) == NULL, "more than two packets sent");
}