int FAST_FUNC udhcp_str2optset(const char *const_str, void *arg)
{
	struct option_set **opt_list = arg;
	char *opt, *val, *endptr;
	char *str;
	const struct dhcp_optflag *optflag;
	struct dhcp_optflag bin_optflag;
	unsigned optcode;
	int retval, length;
	char buffer[8] ALIGNED(4);
	uint16_t *result_u16 = (uint16_t *) buffer;
	uint32_t *result_u32 = (uint32_t *) buffer;

	/* Cheat, the only *const* str possible is "" */
	str = (char *) const_str;
	opt = strtok(str, " \t=");
	if (!opt)
		return 0;

	optcode = bb_strtou(opt, NULL, 0);
	if (!errno && optcode < 255) {
		/* Raw (numeric) option code */
		bin_optflag.flags = OPTION_BIN;
		bin_optflag.code = optcode;
		optflag = &bin_optflag;
	} else {
		optflag = &dhcp_optflags[udhcp_option_idx(opt)];
	}

	retval = 0;
	do {
		val = strtok(NULL, ", \t");
		if (!val)
			break;
		length = dhcp_option_lengths[optflag->flags & OPTION_TYPE_MASK];
		retval = 0;
		opt = buffer; /* new meaning for variable opt */
		switch (optflag->flags & OPTION_TYPE_MASK) {
		case OPTION_IP:
			retval = udhcp_str2nip(val, buffer);
			break;
		case OPTION_IP_PAIR:
			retval = udhcp_str2nip(val, buffer);
			val = strtok(NULL, ", \t/-");
			if (!val)
				retval = 0;
			if (retval)
				retval = udhcp_str2nip(val, buffer + 4);
			break;
		case OPTION_STRING:
#if ENABLE_FEATURE_UDHCP_RFC3397
		case OPTION_DNS_STRING:
#endif
			length = strnlen(val, 254);
			if (length > 0) {
				opt = val;
				retval = 1;
			}
			break;
//		case OPTION_BOOLEAN: {
//			static const char no_yes[] ALIGN1 = "no\0yes\0";
//			buffer[0] = retval = index_in_strings(no_yes, val);
//			retval++; /* 0 - bad; 1: "no" 2: "yes" */
//			break;
//		}
		case OPTION_U8:
			buffer[0] = strtoul(val, &endptr, 0);
			retval = (endptr[0] == '\0');
			break;
		/* htonX are macros in older libc's, using temp var
		 * in code below for safety */
		/* TODO: use bb_strtoX? */
		case OPTION_U16: {
			unsigned long tmp = strtoul(val, &endptr, 0);
			*result_u16 = htons(tmp);
			retval = (endptr[0] == '\0' /*&& tmp < 0x10000*/);
			break;
		}
//		case OPTION_S16: {
//			long tmp = strtol(val, &endptr, 0);
//			*result_u16 = htons(tmp);
//			retval = (endptr[0] == '\0');
//			break;
//		}
		case OPTION_U32: {
			unsigned long tmp = strtoul(val, &endptr, 0);
			*result_u32 = htonl(tmp);
			retval = (endptr[0] == '\0');
			break;
		}
		case OPTION_S32: {
			long tmp = strtol(val, &endptr, 0);
			*result_u32 = htonl(tmp);
			retval = (endptr[0] == '\0');
			break;
		}
		case OPTION_BIN: /* handled in attach_option() */
			opt = val;
			retval = 1;
		default:
			break;
		}
		if (retval)
			attach_option(opt_list, optflag, opt, length);
	} while (retval && optflag->flags & OPTION_LIST);

	return retval;
}