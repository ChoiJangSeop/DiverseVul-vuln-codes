find_available_user()
{
	int ret = -1;
	int i;
	for (i = 0; i < usercount; i++) {
		/* Not used at all or not used in one minute */
		if ((!users[i].active || users[i].last_pkt + 60 < time(NULL)) && !users[i].disabled) {
			users[i].active = 1;
			users[i].last_pkt = time(NULL);
			users[i].fragsize = 4096;
			users[i].conn = CONN_DNS_NULL;
			ret = i;
			break;
		}
	}
	return ret;
}