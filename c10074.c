int iscsi_conn_get_addr_param(struct sockaddr_storage *addr,
			      enum iscsi_param param, char *buf)
{
	struct sockaddr_in6 *sin6 = NULL;
	struct sockaddr_in *sin = NULL;
	int len;

	switch (addr->ss_family) {
	case AF_INET:
		sin = (struct sockaddr_in *)addr;
		break;
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)addr;
		break;
	default:
		return -EINVAL;
	}

	switch (param) {
	case ISCSI_PARAM_CONN_ADDRESS:
	case ISCSI_HOST_PARAM_IPADDRESS:
		if (sin)
			len = sprintf(buf, "%pI4\n", &sin->sin_addr.s_addr);
		else
			len = sprintf(buf, "%pI6\n", &sin6->sin6_addr);
		break;
	case ISCSI_PARAM_CONN_PORT:
	case ISCSI_PARAM_LOCAL_PORT:
		if (sin)
			len = sprintf(buf, "%hu\n", be16_to_cpu(sin->sin_port));
		else
			len = sprintf(buf, "%hu\n",
				      be16_to_cpu(sin6->sin6_port));
		break;
	default:
		return -EINVAL;
	}

	return len;
}