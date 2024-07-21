char *pointer(const char *fmt, char *buf, char *end, void *ptr,
	      struct printf_spec spec)
{
	const int default_width = 2 * sizeof(void *);

	if (!ptr && *fmt != 'K') {
		/*
		 * Print (null) with the same width as a pointer so it makes
		 * tabular output look nice.
		 */
		if (spec.field_width == -1)
			spec.field_width = default_width;
		return string(buf, end, "(null)", spec);
	}

	switch (*fmt) {
	case 'F':
	case 'f':
		ptr = dereference_function_descriptor(ptr);
		/* Fallthrough */
	case 'S':
	case 's':
	case 'B':
		return symbol_string(buf, end, ptr, spec, fmt);
	case 'R':
	case 'r':
		return resource_string(buf, end, ptr, spec, fmt);
	case 'h':
		return hex_string(buf, end, ptr, spec, fmt);
	case 'b':
		switch (fmt[1]) {
		case 'l':
			return bitmap_list_string(buf, end, ptr, spec, fmt);
		default:
			return bitmap_string(buf, end, ptr, spec, fmt);
		}
	case 'M':			/* Colon separated: 00:01:02:03:04:05 */
	case 'm':			/* Contiguous: 000102030405 */
					/* [mM]F (FDDI) */
					/* [mM]R (Reverse order; Bluetooth) */
		return mac_address_string(buf, end, ptr, spec, fmt);
	case 'I':			/* Formatted IP supported
					 * 4:	1.2.3.4
					 * 6:	0001:0203:...:0708
					 * 6c:	1::708 or 1::1.2.3.4
					 */
	case 'i':			/* Contiguous:
					 * 4:	001.002.003.004
					 * 6:   000102...0f
					 */
		switch (fmt[1]) {
		case '6':
			return ip6_addr_string(buf, end, ptr, spec, fmt);
		case '4':
			return ip4_addr_string(buf, end, ptr, spec, fmt);
		case 'S': {
			const union {
				struct sockaddr		raw;
				struct sockaddr_in	v4;
				struct sockaddr_in6	v6;
			} *sa = ptr;

			switch (sa->raw.sa_family) {
			case AF_INET:
				return ip4_addr_string_sa(buf, end, &sa->v4, spec, fmt);
			case AF_INET6:
				return ip6_addr_string_sa(buf, end, &sa->v6, spec, fmt);
			default:
				return string(buf, end, "(invalid address)", spec);
			}}
		}
		break;
	case 'E':
		return escaped_string(buf, end, ptr, spec, fmt);
	case 'U':
		return uuid_string(buf, end, ptr, spec, fmt);
	case 'V':
		{
			va_list va;

			va_copy(va, *((struct va_format *)ptr)->va);
			buf += vsnprintf(buf, end > buf ? end - buf : 0,
					 ((struct va_format *)ptr)->fmt, va);
			va_end(va);
			return buf;
		}
	case 'K':
		return restricted_pointer(buf, end, ptr, spec);
	case 'N':
		return netdev_bits(buf, end, ptr, fmt);
	case 'a':
		return address_val(buf, end, ptr, fmt);
	case 'd':
		return dentry_name(buf, end, ptr, spec, fmt);
	case 'C':
		return clock(buf, end, ptr, spec, fmt);
	case 'D':
		return dentry_name(buf, end,
				   ((const struct file *)ptr)->f_path.dentry,
				   spec, fmt);
#ifdef CONFIG_BLOCK
	case 'g':
		return bdev_name(buf, end, ptr, spec, fmt);
#endif

	case 'G':
		return flags_string(buf, end, ptr, fmt);
	case 'O':
		switch (fmt[1]) {
		case 'F':
			return device_node_string(buf, end, ptr, spec, fmt + 1);
		}
	}
	spec.flags |= SMALL;
	if (spec.field_width == -1) {
		spec.field_width = default_width;
		spec.flags |= ZEROPAD;
	}
	spec.base = 16;

	return number(buf, end, (unsigned long) ptr, spec);
}