static gboolean netscreen_read(wtap *wth, int *err, gchar **err_info,
    gint64 *data_offset)
{
	gint64		offset;
	int		pkt_len;
	char		line[NETSCREEN_LINE_LENGTH];
	char		cap_int[NETSCREEN_MAX_INT_NAME_LENGTH];
	gboolean	cap_dir;
	char		cap_dst[13];

	/* Find the next packet */
	offset = netscreen_seek_next_packet(wth, err, err_info, line);
	if (offset < 0)
		return FALSE;

	/* Parse the header */
	pkt_len = parse_netscreen_rec_hdr(&wth->phdr, line, cap_int, &cap_dir,
	    cap_dst, err, err_info);
	if (pkt_len == -1)
		return FALSE;

	/* Convert the ASCII hex dump to binary data, and fill in some
	   struct wtap_pkthdr fields */
	if (!parse_netscreen_hex_dump(wth->fh, pkt_len, cap_int,
	    cap_dst, &wth->phdr, wth->frame_buffer, err, err_info))
		return FALSE;

	/*
	 * If the per-file encapsulation isn't known, set it to this
	 * packet's encapsulation.
	 *
	 * If it *is* known, and it isn't this packet's encapsulation,
	 * set it to WTAP_ENCAP_PER_PACKET, as this file doesn't
	 * have a single encapsulation for all packets in the file.
	 */
	if (wth->file_encap == WTAP_ENCAP_UNKNOWN)
		wth->file_encap = wth->phdr.pkt_encap;
	else {
		if (wth->file_encap != wth->phdr.pkt_encap)
			wth->file_encap = WTAP_ENCAP_PER_PACKET;
	}

	*data_offset = offset;
	return TRUE;
}