parse_toshiba_packet(FILE_T fh, struct wtap_pkthdr *phdr, Buffer *buf,
    int *err, gchar **err_info)
{
	union wtap_pseudo_header *pseudo_header = &phdr->pseudo_header;
	char	line[TOSHIBA_LINE_LENGTH];
	int	num_items_scanned;
	guint	pkt_len;
	int	pktnum, hr, min, sec, csec;
	char	channel[10], direction[10];
	int	i, hex_lines;
	guint8	*pd;

	/* Our file pointer should be on the line containing the
	 * summary information for a packet. Read in that line and
	 * extract the useful information
	 */
	if (file_gets(line, TOSHIBA_LINE_LENGTH, fh) == NULL) {
		*err = file_error(fh, err_info);
		if (*err == 0) {
			*err = WTAP_ERR_SHORT_READ;
		}
		return FALSE;
	}

	/* Find text in line after "[No.". Limit the length of the
	 * two strings since we have fixed buffers for channel[] and
	 * direction[] */
	num_items_scanned = sscanf(line, "%9d] %2d:%2d:%2d.%9d %9s %9s",
			&pktnum, &hr, &min, &sec, &csec, channel, direction);

	if (num_items_scanned != 7) {
		*err = WTAP_ERR_BAD_FILE;
		*err_info = g_strdup("toshiba: record header isn't valid");
		return FALSE;
	}

	/* Scan lines until we find the OFFSET line. In a "telnet" trace,
	 * this will be the next line. But if you save your telnet session
	 * to a file from within a Windows-based telnet client, it may
	 * put in line breaks at 80 columns (or however big your "telnet" box
	 * is). CRT (a Windows telnet app from VanDyke) does this.
	 * Here we assume that 80 columns will be the minimum size, and that
	 * the OFFSET line is not broken in the middle. It's the previous
	 * line that is normally long and can thus be broken at column 80.
	 */
	do {
		if (file_gets(line, TOSHIBA_LINE_LENGTH, fh) == NULL) {
			*err = file_error(fh, err_info);
			if (*err == 0) {
				*err = WTAP_ERR_SHORT_READ;
			}
			return FALSE;
		}

		/* Check for "OFFSET 0001-0203" at beginning of line */
		line[16] = '\0';

	} while (strcmp(line, "OFFSET 0001-0203") != 0);

	num_items_scanned = sscanf(line+64, "LEN=%9u", &pkt_len);
	if (num_items_scanned != 1) {
		*err = WTAP_ERR_BAD_FILE;
		*err_info = g_strdup("toshiba: OFFSET line doesn't have valid LEN item");
		return FALSE;
	}
	if (pkt_len > WTAP_MAX_PACKET_SIZE) {
		/*
		 * Probably a corrupt capture file; don't blow up trying
		 * to allocate space for an immensely-large packet.
		 */
		*err = WTAP_ERR_BAD_FILE;
		*err_info = g_strdup_printf("toshiba: File has %u-byte packet, bigger than maximum of %u",
		    pkt_len, WTAP_MAX_PACKET_SIZE);
		return FALSE;
	}

	phdr->rec_type = REC_TYPE_PACKET;
	phdr->presence_flags = WTAP_HAS_TS|WTAP_HAS_CAP_LEN;
	phdr->ts.secs = hr * 3600 + min * 60 + sec;
	phdr->ts.nsecs = csec * 10000000;
	phdr->caplen = pkt_len;
	phdr->len = pkt_len;

	switch (channel[0]) {
		case 'B':
			phdr->pkt_encap = WTAP_ENCAP_ISDN;
			pseudo_header->isdn.uton = (direction[0] == 'T');
			pseudo_header->isdn.channel = (guint8)
			    strtol(&channel[1], NULL, 10);
			break;

		case 'D':
			phdr->pkt_encap = WTAP_ENCAP_ISDN;
			pseudo_header->isdn.uton = (direction[0] == 'T');
			pseudo_header->isdn.channel = 0;
			break;

		default:
			phdr->pkt_encap = WTAP_ENCAP_ETHERNET;
			/* XXX - is there an FCS in the frame? */
			pseudo_header->eth.fcs_len = -1;
			break;
	}

	/* Make sure we have enough room for the packet */
	ws_buffer_assure_space(buf, pkt_len);
	pd = ws_buffer_start_ptr(buf);

	/* Calculate the number of hex dump lines, each
	 * containing 16 bytes of data */
	hex_lines = pkt_len / 16 + ((pkt_len % 16) ? 1 : 0);

	for (i = 0; i < hex_lines; i++) {
		if (file_gets(line, TOSHIBA_LINE_LENGTH, fh) == NULL) {
			*err = file_error(fh, err_info);
			if (*err == 0) {
				*err = WTAP_ERR_SHORT_READ;
			}
			return FALSE;
		}
		if (!parse_single_hex_dump_line(line, pd, i * 16)) {
			*err = WTAP_ERR_BAD_FILE;
			*err_info = g_strdup("toshiba: hex dump not valid");
			return FALSE;
		}
	}
	return TRUE;
}