parse_cosine_packet(FILE_T fh, struct wtap_pkthdr *phdr, Buffer *buf,
    char *line, int *err, gchar **err_info)
{
	union wtap_pseudo_header *pseudo_header = &phdr->pseudo_header;
	int	num_items_scanned;
	int	yy, mm, dd, hr, min, sec, csec;
	guint	pkt_len;
	int	pro, off, pri, rm, error;
	guint	code1, code2;
	char	if_name[COSINE_MAX_IF_NAME_LEN] = "", direction[6] = "";
	struct	tm tm;
	guint8 *pd;
	int	i, hex_lines, n, caplen = 0;

	if (sscanf(line, "%4d-%2d-%2d,%2d:%2d:%2d.%9d:",
		   &yy, &mm, &dd, &hr, &min, &sec, &csec) == 7) {
		/* appears to be output to a control blade */
		num_items_scanned = sscanf(line,
		   "%4d-%2d-%2d,%2d:%2d:%2d.%9d: %5s (%127[A-Za-z0-9/:]), Length:%9u, Pro:%9d, Off:%9d, Pri:%9d, RM:%9d, Err:%9d [%8x, %8x]",
			&yy, &mm, &dd, &hr, &min, &sec, &csec,
				   direction, if_name, &pkt_len,
				   &pro, &off, &pri, &rm, &error,
				   &code1, &code2);

		if (num_items_scanned != 17) {
			*err = WTAP_ERR_BAD_FILE;
			*err_info = g_strdup("cosine: purported control blade line doesn't have code values");
			return FALSE;
		}
	} else {
		/* appears to be output to PE */
		num_items_scanned = sscanf(line,
		   "%5s (%127[A-Za-z0-9/:]), Length:%9u, Pro:%9d, Off:%9d, Pri:%9d, RM:%9d, Err:%9d [%8x, %8x]",
				   direction, if_name, &pkt_len,
				   &pro, &off, &pri, &rm, &error,
				   &code1, &code2);

		if (num_items_scanned != 10) {
			*err = WTAP_ERR_BAD_FILE;
			*err_info = g_strdup("cosine: header line is neither control blade nor PE output");
			return FALSE;
		}
		yy = mm = dd = hr = min = sec = csec = 0;
	}
	if (pkt_len > WTAP_MAX_PACKET_SIZE) {
		/*
		 * Probably a corrupt capture file; don't blow up trying
		 * to allocate space for an immensely-large packet.
		 */
		*err = WTAP_ERR_BAD_FILE;
		*err_info = g_strdup_printf("cosine: File has %u-byte packet, bigger than maximum of %u",
		    pkt_len, WTAP_MAX_PACKET_SIZE);
		return FALSE;
	}

	phdr->rec_type = REC_TYPE_PACKET;
	phdr->presence_flags = WTAP_HAS_TS|WTAP_HAS_CAP_LEN;
	tm.tm_year = yy - 1900;
	tm.tm_mon = mm - 1;
	tm.tm_mday = dd;
	tm.tm_hour = hr;
	tm.tm_min = min;
	tm.tm_sec = sec;
	tm.tm_isdst = -1;
	phdr->ts.secs = mktime(&tm);
	phdr->ts.nsecs = csec * 10000000;
	phdr->len = pkt_len;

	/* XXX need to handle other encapsulations like Cisco HDLC,
	   Frame Relay and ATM */
	if (strncmp(if_name, "TEST:", 5) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_TEST;
	} else if (strncmp(if_name, "PPoATM:", 7) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_PPoATM;
	} else if (strncmp(if_name, "PPoFR:", 6) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_PPoFR;
	} else if (strncmp(if_name, "ATM:", 4) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_ATM;
	} else if (strncmp(if_name, "FR:", 3) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_FR;
	} else if (strncmp(if_name, "HDLC:", 5) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_HDLC;
	} else if (strncmp(if_name, "PPP:", 4) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_PPP;
	} else if (strncmp(if_name, "ETH:", 4) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_ETH;
	} else {
		pseudo_header->cosine.encap = COSINE_ENCAP_UNKNOWN;
	}
	if (strncmp(direction, "l2-tx", 5) == 0) {
		pseudo_header->cosine.direction = COSINE_DIR_TX;
	} else if (strncmp(direction, "l2-rx", 5) == 0) {
		pseudo_header->cosine.direction = COSINE_DIR_RX;
	}
	g_strlcpy(pseudo_header->cosine.if_name, if_name,
		COSINE_MAX_IF_NAME_LEN);
	pseudo_header->cosine.pro = pro;
	pseudo_header->cosine.off = off;
	pseudo_header->cosine.pri = pri;
	pseudo_header->cosine.rm = rm;
	pseudo_header->cosine.err = error;

	/* Make sure we have enough room for the packet */
	ws_buffer_assure_space(buf, pkt_len);
	pd = ws_buffer_start_ptr(buf);

	/* Calculate the number of hex dump lines, each
	 * containing 16 bytes of data */
	hex_lines = pkt_len / 16 + ((pkt_len % 16) ? 1 : 0);

	for (i = 0; i < hex_lines; i++) {
		if (file_gets(line, COSINE_LINE_LENGTH, fh) == NULL) {
			*err = file_error(fh, err_info);
			if (*err == 0) {
				*err = WTAP_ERR_SHORT_READ;
			}
			return FALSE;
		}
		if (empty_line(line)) {
			break;
		}
		if ((n = parse_single_hex_dump_line(line, pd, i*16)) == -1) {
			*err = WTAP_ERR_BAD_FILE;
			*err_info = g_strdup("cosine: hex dump line doesn't have 16 numbers");
			return FALSE;
		}
		caplen += n;
	}
	phdr->caplen = caplen;
	return TRUE;
}