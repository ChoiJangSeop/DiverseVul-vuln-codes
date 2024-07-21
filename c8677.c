static int addr6_resolve(struct sockaddr *src_sock,
			 const struct sockaddr *dst_sock,
			 struct rdma_dev_addr *addr,
			 struct dst_entry **pdst)
{
	struct sockaddr_in6 *src_in = (struct sockaddr_in6 *)src_sock;
	const struct sockaddr_in6 *dst_in =
				(const struct sockaddr_in6 *)dst_sock;
	struct flowi6 fl6;
	struct dst_entry *dst;
	int ret;

	memset(&fl6, 0, sizeof fl6);
	fl6.daddr = dst_in->sin6_addr;
	fl6.saddr = src_in->sin6_addr;
	fl6.flowi6_oif = addr->bound_dev_if;

	ret = ipv6_stub->ipv6_dst_lookup(addr->net, NULL, &dst, &fl6);
	if (ret < 0)
		return ret;

	if (ipv6_addr_any(&src_in->sin6_addr))
		src_in->sin6_addr = fl6.saddr;

	addr->hoplimit = ip6_dst_hoplimit(dst);

	*pdst = dst;
	return 0;
}