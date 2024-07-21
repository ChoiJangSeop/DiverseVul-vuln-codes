int ping_common_sendmsg(int family, struct msghdr *msg, size_t len,
			void *user_icmph, size_t icmph_len) {
	u8 type, code;

	if (len > 0xFFFF)
		return -EMSGSIZE;

	/*
	 *	Check the flags.
	 */

	/* Mirror BSD error message compatibility */
	if (msg->msg_flags & MSG_OOB)
		return -EOPNOTSUPP;

	/*
	 *	Fetch the ICMP header provided by the userland.
	 *	iovec is modified! The ICMP header is consumed.
	 */
	if (memcpy_from_msg(user_icmph, msg, icmph_len))
		return -EFAULT;

	if (family == AF_INET) {
		type = ((struct icmphdr *) user_icmph)->type;
		code = ((struct icmphdr *) user_icmph)->code;
#if IS_ENABLED(CONFIG_IPV6)
	} else if (family == AF_INET6) {
		type = ((struct icmp6hdr *) user_icmph)->icmp6_type;
		code = ((struct icmp6hdr *) user_icmph)->icmp6_code;
#endif
	} else {
		BUG();
	}

	if (!ping_supported(family, type, code))
		return -EINVAL;

	return 0;
}