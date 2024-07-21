uint8_t *dhcp_get_option(struct dhcp_packet *packet, int code)
{
	int len, rem;
	uint8_t *optionptr;
	uint8_t overload = 0;

	/* option bytes: [code][len][data1][data2]..[dataLEN] */
	optionptr = packet->options;
	rem = sizeof(packet->options);

	while (1) {
		if (rem <= 0)
			/* Bad packet, malformed option field */
			return NULL;

		if (optionptr[OPT_CODE] == DHCP_PADDING) {
			rem--;
			optionptr++;

			continue;
		}

		if (optionptr[OPT_CODE] == DHCP_END) {
			if (overload & FILE_FIELD) {
				overload &= ~FILE_FIELD;

				optionptr = packet->file;
				rem = sizeof(packet->file);

				continue;
			} else if (overload & SNAME_FIELD) {
				overload &= ~SNAME_FIELD;

				optionptr = packet->sname;
				rem = sizeof(packet->sname);

				continue;
			}

			break;
		}

		len = 2 + optionptr[OPT_LEN];

		rem -= len;
		if (rem < 0)
			continue; /* complain and return NULL */

		if (optionptr[OPT_CODE] == code)
			return optionptr + OPT_DATA;

		if (optionptr[OPT_CODE] == DHCP_OPTION_OVERLOAD)
			overload |= optionptr[OPT_DATA];

		optionptr += len;
	}

	return NULL;
}