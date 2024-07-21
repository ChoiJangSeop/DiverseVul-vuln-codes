int ip_options_get(struct net *net, struct ip_options **optp,
		   unsigned char *data, int optlen)
{
	struct ip_options *opt = ip_options_get_alloc(optlen);

	if (!opt)
		return -ENOMEM;
	if (optlen)
		memcpy(opt->__data, data, optlen);
	return ip_options_get_finish(net, optp, opt, optlen);
}