static gboolean cosine_read(wtap *wth, int *err, gchar **err_info,
    gint64 *data_offset)
{
	gint64	offset;
	int	pkt_len;
	char	line[COSINE_LINE_LENGTH];

	/* Find the next packet */
	offset = cosine_seek_next_packet(wth, err, err_info, line);
	if (offset < 0)
		return FALSE;
	*data_offset = offset;

	/* Parse the header */
	pkt_len = parse_cosine_rec_hdr(&wth->phdr, line, err, err_info);
	if (pkt_len == -1)
		return FALSE;

	/* Convert the ASCII hex dump to binary data */
	return parse_cosine_hex_dump(wth->fh, &wth->phdr, pkt_len,
	    wth->frame_buffer, err, err_info);
}