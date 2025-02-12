void rds6_inc_info_copy(struct rds_incoming *inc,
			struct rds_info_iterator *iter,
			struct in6_addr *saddr, struct in6_addr *daddr,
			int flip)
{
	struct rds6_info_message minfo6;

	minfo6.seq = be64_to_cpu(inc->i_hdr.h_sequence);
	minfo6.len = be32_to_cpu(inc->i_hdr.h_len);

	if (flip) {
		minfo6.laddr = *daddr;
		minfo6.faddr = *saddr;
		minfo6.lport = inc->i_hdr.h_dport;
		minfo6.fport = inc->i_hdr.h_sport;
	} else {
		minfo6.laddr = *saddr;
		minfo6.faddr = *daddr;
		minfo6.lport = inc->i_hdr.h_sport;
		minfo6.fport = inc->i_hdr.h_dport;
	}

	rds_info_copy(iter, &minfo6, sizeof(minfo6));
}