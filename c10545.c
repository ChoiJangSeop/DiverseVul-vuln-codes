static int validate_tmpl(int nr, struct xfrm_user_tmpl *ut, u16 family)
{
	u16 prev_family;
	int i;

	if (nr > XFRM_MAX_DEPTH)
		return -EINVAL;

	prev_family = family;

	for (i = 0; i < nr; i++) {
		/* We never validated the ut->family value, so many
		 * applications simply leave it at zero.  The check was
		 * never made and ut->family was ignored because all
		 * templates could be assumed to have the same family as
		 * the policy itself.  Now that we will have ipv4-in-ipv6
		 * and ipv6-in-ipv4 tunnels, this is no longer true.
		 */
		if (!ut[i].family)
			ut[i].family = family;

		switch (ut[i].mode) {
		case XFRM_MODE_TUNNEL:
		case XFRM_MODE_BEET:
			break;
		default:
			if (ut[i].family != prev_family)
				return -EINVAL;
			break;
		}
		if (ut[i].mode >= XFRM_MODE_MAX)
			return -EINVAL;

		prev_family = ut[i].family;

		switch (ut[i].family) {
		case AF_INET:
			break;
#if IS_ENABLED(CONFIG_IPV6)
		case AF_INET6:
			break;
#endif
		default:
			return -EINVAL;
		}

		switch (ut[i].id.proto) {
		case IPPROTO_AH:
		case IPPROTO_ESP:
		case IPPROTO_COMP:
#if IS_ENABLED(CONFIG_IPV6)
		case IPPROTO_ROUTING:
		case IPPROTO_DSTOPTS:
#endif
		case IPSEC_PROTO_ANY:
			break;
		default:
			return -EINVAL;
		}

	}

	return 0;
}