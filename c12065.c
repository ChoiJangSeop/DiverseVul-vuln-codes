static int rose_parse_ccitt(unsigned char *p, struct rose_facilities_struct *facilities, int len)
{
	unsigned char l, n = 0;
	char callsign[11];

	do {
		switch (*p & 0xC0) {
		case 0x00:
			p   += 2;
			n   += 2;
			len -= 2;
			break;

		case 0x40:
			p   += 3;
			n   += 3;
			len -= 3;
			break;

		case 0x80:
			p   += 4;
			n   += 4;
			len -= 4;
			break;

		case 0xC0:
			l = p[1];

			/* Prevent overflows*/
			if (l < 10 || l > 20)
				return -1;

			if (*p == FAC_CCITT_DEST_NSAP) {
				memcpy(&facilities->source_addr, p + 7, ROSE_ADDR_LEN);
				memcpy(callsign, p + 12,   l - 10);
				callsign[l - 10] = '\0';
				asc2ax(&facilities->source_call, callsign);
			}
			if (*p == FAC_CCITT_SRC_NSAP) {
				memcpy(&facilities->dest_addr, p + 7, ROSE_ADDR_LEN);
				memcpy(callsign, p + 12, l - 10);
				callsign[l - 10] = '\0';
				asc2ax(&facilities->dest_call, callsign);
			}
			p   += l + 2;
			n   += l + 2;
			len -= l + 2;
			break;
		}
	} while (*p != 0x00 && len > 0);

	return n;
}