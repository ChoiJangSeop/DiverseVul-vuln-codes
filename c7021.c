uint8_t* FAST_FUNC udhcp_get_option(struct dhcp_packet *packet, int code)
{
	uint8_t *optionptr;
	int len;
	int rem;
	int overload = 0;
	enum {
		FILE_FIELD101  = FILE_FIELD  * 0x101,
		SNAME_FIELD101 = SNAME_FIELD * 0x101,
	};

	/* option bytes: [code][len][data1][data2]..[dataLEN] */
	optionptr = packet->options;
	rem = sizeof(packet->options);
	while (1) {
		if (rem <= 0) {
 complain:
			bb_error_msg("bad packet, malformed option field");
			return NULL;
		}

		/* DHCP_PADDING and DHCP_END have no [len] byte */
		if (optionptr[OPT_CODE] == DHCP_PADDING) {
			rem--;
			optionptr++;
			continue;
		}
		if (optionptr[OPT_CODE] == DHCP_END) {
			if ((overload & FILE_FIELD101) == FILE_FIELD) {
				/* can use packet->file, and didn't look at it yet */
				overload |= FILE_FIELD101; /* "we looked at it" */
				optionptr = packet->file;
				rem = sizeof(packet->file);
				continue;
			}
			if ((overload & SNAME_FIELD101) == SNAME_FIELD) {
				/* can use packet->sname, and didn't look at it yet */
				overload |= SNAME_FIELD101; /* "we looked at it" */
				optionptr = packet->sname;
				rem = sizeof(packet->sname);
				continue;
			}
			break;
		}

		if (rem <= OPT_LEN)
			goto complain; /* complain and return NULL */
		len = 2 + optionptr[OPT_LEN];
		rem -= len;
		if (rem < 0)
			goto complain; /* complain and return NULL */

		if (optionptr[OPT_CODE] == code) {
			log_option("option found", optionptr);
			return optionptr + OPT_DATA;
		}

		if (optionptr[OPT_CODE] == DHCP_OPTION_OVERLOAD) {
			if (len >= 3)
				overload |= optionptr[OPT_DATA];
			/* fall through */
		}
		optionptr += len;
	}

	/* log3 because udhcpc uses it a lot - very noisy */
	log3("option 0x%02x not found", code);
	return NULL;
}