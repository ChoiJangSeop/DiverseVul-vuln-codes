GIOChannel *net_connect_ip_ssl(IPADDR *ip, int port, IPADDR *my_ip, const char *cert, const char *pkey, const char *cafile, const char *capath, gboolean verify)
{
	g_warning("Connection failed: SSL support not enabled in this build.");
	errno = ENOSYS;
	return NULL;
}