init_users(in_addr_t my_ip, int netbits)
{
	int i;
	int skip = 0;
	char newip[16];

	int maxusers;

	in_addr_t netmask = 0;
	struct in_addr net;
	struct in_addr ipstart;

	for (i = 0; i < netbits; i++) {
		netmask = (netmask << 1) | 1;
	}
	netmask <<= (32 - netbits);
	net.s_addr = htonl(netmask);
	ipstart.s_addr = my_ip & net.s_addr;

	maxusers = (1 << (32-netbits)) - 3; /* 3: Net addr, broadcast addr, iodined addr */
	usercount = MIN(maxusers, USERS);

	users = calloc(usercount, sizeof(struct tun_user));
	for (i = 0; i < usercount; i++) {
		in_addr_t ip;
		users[i].id = i;
		snprintf(newip, sizeof(newip), "0.0.0.%d", i + skip + 1);
		ip = ipstart.s_addr + inet_addr(newip);
		if (ip == my_ip && skip == 0) {
			/* This IP was taken by iodined */
			skip++;
			snprintf(newip, sizeof(newip), "0.0.0.%d", i + skip + 1);
			ip = ipstart.s_addr + inet_addr(newip);
		}
		users[i].tun_ip = ip;
		net.s_addr = ip;
		users[i].disabled = 0;
		users[i].active = 0;
 		/* Rest is reset on login ('V' packet) */
	}

	return usercount;
}