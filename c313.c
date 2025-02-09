parse_tag_11_packet(unsigned char *data, unsigned char *contents,
		    size_t max_contents_bytes, size_t *tag_11_contents_size,
		    size_t *packet_size, size_t max_packet_size)
{
	size_t body_size;
	size_t length_size;
	int rc = 0;

	(*packet_size) = 0;
	(*tag_11_contents_size) = 0;
	/* This format is inspired by OpenPGP; see RFC 2440
	 * packet tag 11
	 *
	 * Tag 11 identifier (1 byte)
	 * Max Tag 11 packet size (max 3 bytes)
	 * Binary format specifier (1 byte)
	 * Filename length (1 byte)
	 * Filename ("_CONSOLE") (8 bytes)
	 * Modification date (4 bytes)
	 * Literal data (arbitrary)
	 *
	 * We need at least 16 bytes of data for the packet to even be
	 * valid.
	 */
	if (max_packet_size < 16) {
		printk(KERN_ERR "Maximum packet size too small\n");
		rc = -EINVAL;
		goto out;
	}
	if (data[(*packet_size)++] != ECRYPTFS_TAG_11_PACKET_TYPE) {
		printk(KERN_WARNING "Invalid tag 11 packet format\n");
		rc = -EINVAL;
		goto out;
	}
	rc = ecryptfs_parse_packet_length(&data[(*packet_size)], &body_size,
					  &length_size);
	if (rc) {
		printk(KERN_WARNING "Invalid tag 11 packet format\n");
		goto out;
	}
	if (body_size < 14) {
		printk(KERN_WARNING "Invalid body size ([%td])\n", body_size);
		rc = -EINVAL;
		goto out;
	}
	(*packet_size) += length_size;
	(*tag_11_contents_size) = (body_size - 14);
	if (unlikely((*packet_size) + body_size + 1 > max_packet_size)) {
		printk(KERN_ERR "Packet size exceeds max\n");
		rc = -EINVAL;
		goto out;
	}
	if (data[(*packet_size)++] != 0x62) {
		printk(KERN_WARNING "Unrecognizable packet\n");
		rc = -EINVAL;
		goto out;
	}
	if (data[(*packet_size)++] != 0x08) {
		printk(KERN_WARNING "Unrecognizable packet\n");
		rc = -EINVAL;
		goto out;
	}
	(*packet_size) += 12; /* Ignore filename and modification date */
	memcpy(contents, &data[(*packet_size)], (*tag_11_contents_size));
	(*packet_size) += (*tag_11_contents_size);
out:
	if (rc) {
		(*packet_size) = 0;
		(*tag_11_contents_size) = 0;
	}
	return rc;
}