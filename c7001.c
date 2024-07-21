static int qh_echo(int sd, char *buf, unsigned int len)
{
	int result = 0;

	if (!strcmp(buf, "help")) {

		nsock_printf_nul(sd,
			"Query handler that simply echoes back what you send it.");
		return 0;
	}

	result = write(sd, buf, len);
	if (result == -1) {

		logit(NSLOG_RUNTIME_ERROR, TRUE, "qh: qh_echo() error on write(sd,buf=[%s],len=%d): %s\n", buf, len, strerror(errno));
	}
	return 0;
}