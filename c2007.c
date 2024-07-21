find_user_by_ip(uint32_t ip)
{
	int ret;
	int i;

	ret = -1;
	for (i = 0; i < usercount; i++) {
		if (users[i].active && !users[i].disabled &&
			users[i].last_pkt + 60 > time(NULL) &&
			ip == users[i].tun_ip) {
			ret = i;
			break;
		}
	}
	return ret;
}