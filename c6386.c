static FILE* prepare_ftp_session(FILE **dfpp, struct host_info *target, len_and_sockaddr *lsa)
{
	FILE *sfp;
	char *pass;
	int port;

	sfp = open_socket(lsa);
#if ENABLE_FEATURE_WGET_HTTPS
	if (target->protocol == P_FTPS)
		spawn_ssl_client(target->host, fileno(sfp), TLSLOOP_EXIT_ON_LOCAL_EOF);
#endif

	if (ftpcmd(NULL, NULL, sfp) != 220)
		bb_error_msg_and_die("%s", G.wget_buf);
		/* note: ftpcmd() sanitizes G.wget_buf, ok to print */

	/* Split username:password pair */
	pass = (char*)"busybox"; /* password for "anonymous" */
	if (target->user) {
		pass = strchr(target->user, ':');
		if (pass)
			*pass++ = '\0';
	}

	/* Log in */
	switch (ftpcmd("USER ", target->user ?: "anonymous", sfp)) {
	case 230:
		break;
	case 331:
		if (ftpcmd("PASS ", pass, sfp) == 230)
			break;
		/* fall through (failed login) */
	default:
		bb_error_msg_and_die("ftp login: %s", G.wget_buf);
	}

	ftpcmd("TYPE I", NULL, sfp);

	/* Query file size */
	if (ftpcmd("SIZE ", target->path, sfp) == 213) {
		G.content_len = BB_STRTOOFF(G.wget_buf + 4, NULL, 10);
		if (G.content_len < 0 || errno) {
			bb_error_msg_and_die("SIZE value is garbage");
		}
		G.got_clen = 1;
	}

	/* Enter passive mode */
	if (ENABLE_FEATURE_IPV6 && ftpcmd("EPSV", NULL, sfp) == 229) {
		/* good */
	} else
	if (ftpcmd("PASV", NULL, sfp) != 227) {
 pasv_error:
		bb_error_msg_and_die("bad response to %s: %s", "PASV", G.wget_buf);
	}
	port = parse_pasv_epsv(G.wget_buf);
	if (port < 0)
		goto pasv_error;

	set_nport(&lsa->u.sa, htons(port));

	*dfpp = open_socket(lsa);

#if ENABLE_FEATURE_WGET_HTTPS
	if (target->protocol == P_FTPS) {
		/* "PROT P" enables encryption of data stream.
		 * Without it (or with "PROT C"), data is sent unencrypted.
		 */
		if (ftpcmd("PROT P", NULL, sfp) == 200)
			spawn_ssl_client(target->host, fileno(*dfpp), /*flags*/ 0);
	}
#endif

	if (G.beg_range != 0) {
		sprintf(G.wget_buf, "REST %"OFF_FMT"u", G.beg_range);
		if (ftpcmd(G.wget_buf, NULL, sfp) == 350)
			G.content_len -= G.beg_range;
		else
			reset_beg_range_to_zero();
	}

//TODO: needs ftp-escaping 0xff and '\n' bytes here.
//Or disallow '\n' altogether via sanitize_string() in parse_url().
//But 0xff's are possible in valid utf8 filenames.
	if (ftpcmd("RETR ", target->path, sfp) > 150)
		bb_error_msg_and_die("bad response to %s: %s", "RETR", G.wget_buf);

	return sfp;
}