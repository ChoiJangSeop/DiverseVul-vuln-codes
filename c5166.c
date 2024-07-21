static int parse_sockaddr_pair(struct sockaddr *sa, int ext_len,
			       xfrm_address_t *saddr, xfrm_address_t *daddr,
			       u16 *family)
{
	int af, socklen;

	if (ext_len < pfkey_sockaddr_pair_size(sa->sa_family))
		return -EINVAL;

	af = pfkey_sockaddr_extract(sa, saddr);
	if (!af)
		return -EINVAL;

	socklen = pfkey_sockaddr_len(af);
	if (pfkey_sockaddr_extract((struct sockaddr *) (((u8 *)sa) + socklen),
				   daddr) != af)
		return -EINVAL;

	*family = af;
	return 0;
}