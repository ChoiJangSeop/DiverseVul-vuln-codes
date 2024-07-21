handle_raw_login(char *packet, int len, struct query *q, int fd, int userid)
{
	char myhash[16];

	if (len < 16) return;

	/* can't use check_user_and_ip() since IP address will be different,
	   so duplicate here except IP address */
	if (userid < 0 || userid >= created_users) return;
	if (!users[userid].active || users[userid].disabled) return;
	if (users[userid].last_pkt + 60 < time(NULL)) return;

	if (debug >= 1) {
		fprintf(stderr, "IN   login raw, len %d, from user %d\n",
			len, userid);
	}

	/* User sends hash of seed + 1 */
	login_calculate(myhash, 16, password, users[userid].seed + 1);
	if (memcmp(packet, myhash, 16) == 0) {
		struct sockaddr_in *tempin;

		/* Update query and time info for user */
		users[userid].last_pkt = time(NULL);
		memcpy(&(users[userid].q), q, sizeof(struct query));

		/* Store remote IP number */
		tempin = (struct sockaddr_in *) &(q->from);
		memcpy(&(users[userid].host), &(tempin->sin_addr), sizeof(struct in_addr));

		/* Correct hash, reply with hash of seed - 1 */
		user_set_conn_type(userid, CONN_RAW_UDP);
		login_calculate(myhash, 16, password, users[userid].seed - 1);
		send_raw(fd, myhash, 16, userid, RAW_HDR_CMD_LOGIN, q);
	}
}