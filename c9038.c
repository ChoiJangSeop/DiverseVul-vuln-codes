exif_mnote_data_olympus_load (ExifMnoteData *en,
			      const unsigned char *buf, unsigned int buf_size)
{
	ExifMnoteDataOlympus *n = (ExifMnoteDataOlympus *) en;
	ExifShort c;
	size_t i, tcount, o, o2, datao = 6, base = 0;

	if (!n || !buf || !buf_size) {
		exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
			  "ExifMnoteDataOlympus", "Short MakerNote");
		return;
	}
	o2 = 6 + n->offset; /* Start of interesting data */
	if (CHECKOVERFLOW(o2,buf_size,10)) {
		exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
			  "ExifMnoteDataOlympus", "Short MakerNote");
		return;
	}

	/*
	 * Olympus headers start with "OLYMP" and need to have at least
	 * a size of 22 bytes (6 for 'OLYMP', 2 other bytes, 2 for the
	 * number of entries, and 12 for one entry.
	 *
	 * Sanyo format is identical and uses identical tags except that
	 * header starts with "SANYO".
	 *
	 * Epson format is identical and uses identical tags except that
	 * header starts with "EPSON".
	 *
	 * Nikon headers start with "Nikon" (6 bytes including '\0'), 
	 * version number (1 or 2).
	 * 
	 * Version 1 continues with 0, 1, 0, number_of_tags,
	 * or just with number_of_tags (models D1H, D1X...).
	 * 
	 * Version 2 continues with an unknown byte (0 or 10),
	 * two unknown bytes (0), "MM" or "II", another byte 0 and 
	 * lastly 0x2A.
	 */
	n->version = exif_mnote_data_olympus_identify_variant(buf+o2, buf_size-o2);
	switch (n->version) {
	case olympusV1:
	case sanyoV1:
	case epsonV1:
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataOlympus",
			"Parsing Olympus/Sanyo/Epson maker note v1...");

		/* The number of entries is at position 8. */
		if (buf[o2 + 6] == 1)
			n->order = EXIF_BYTE_ORDER_INTEL;
		else if (buf[o2 + 6 + 1] == 1)
			n->order = EXIF_BYTE_ORDER_MOTOROLA;
		o2 += 8;
		c = exif_get_short (buf + o2, n->order);
		if ((!(c & 0xFF)) && (c > 0x500)) {
			if (n->order == EXIF_BYTE_ORDER_INTEL) {
				n->order = EXIF_BYTE_ORDER_MOTOROLA;
			} else {
				n->order = EXIF_BYTE_ORDER_INTEL;
			}
		}
		break;

	case olympusV2:
		/* Olympus S760, S770 */
		datao = o2;
		o2 += 8;
		if (CHECKOVERFLOW(o2,buf_size,4)) return;
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataOlympus",
			"Parsing Olympus maker note v2 (0x%02x, %02x, %02x, %02x)...",
			buf[o2 + 0], buf[o2 + 1], buf[o2 + 2], buf[o2 + 3]);

		if ((buf[o2] == 'I') && (buf[o2 + 1] == 'I'))
			n->order = EXIF_BYTE_ORDER_INTEL;
		else if ((buf[o2] == 'M') && (buf[o2 + 1] == 'M'))
			n->order = EXIF_BYTE_ORDER_MOTOROLA;

		/* The number of entries is at position 8+4. */
		o2 += 4;
		break;

	case nikonV1:
		o2 += 6;
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataOlympus",
			"Parsing Nikon maker note v1 (0x%02x, %02x, %02x, "
			"%02x)...",
			buf[o2 + 0], buf[o2 + 1], buf[o2 + 2], buf[o2 + 3]);

		/* Skip version number */
		o2 += 1;

		/* Skip an unknown byte (00 or 0A). */
		o2 += 1;

		base = MNOTE_NIKON1_TAG_BASE;
		/* Fix endianness, if needed */
		c = exif_get_short (buf + o2, n->order);
		if ((!(c & 0xFF)) && (c > 0x500)) {
			if (n->order == EXIF_BYTE_ORDER_INTEL) {
				n->order = EXIF_BYTE_ORDER_MOTOROLA;
			} else {
				n->order = EXIF_BYTE_ORDER_INTEL;
			}
		}
		break;

	case nikonV2:
		o2 += 6;
		if (CHECKOVERFLOW(o2,buf_size,12)) return;
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataOlympus",
			"Parsing Nikon maker note v2 (0x%02x, %02x, %02x, "
			"%02x, %02x, %02x, %02x, %02x)...",
			buf[o2 + 0], buf[o2 + 1], buf[o2 + 2], buf[o2 + 3],
			buf[o2 + 4], buf[o2 + 5], buf[o2 + 6], buf[o2 + 7]);

		/* Skip version number */
		o2 += 1;

		/* Skip an unknown byte (00 or 0A). */
		o2 += 1;

		/* Skip 2 unknown bytes (00 00). */
		o2 += 2;

		/*
		 * Byte order. From here the data offset
		 * gets calculated.
		 */
		datao = o2;
		if (!strncmp ((char *)&buf[o2], "II", 2))
			n->order = EXIF_BYTE_ORDER_INTEL;
		else if (!strncmp ((char *)&buf[o2], "MM", 2))
			n->order = EXIF_BYTE_ORDER_MOTOROLA;
		else {
			exif_log (en->log, EXIF_LOG_CODE_DEBUG,
				"ExifMnoteDataOlympus", "Unknown "
				"byte order '%c%c'", buf[o2],
				buf[o2 + 1]);
			return;
		}
		o2 += 2;

		/* Skip 2 unknown bytes (00 2A). */
		o2 += 2;

		/* Go to where the number of entries is. */
		o2 = datao + exif_get_long (buf + o2, n->order);
		break;

	case nikonV0:
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataOlympus",
			"Parsing Nikon maker note v0 (0x%02x, %02x, %02x, "
			"%02x, %02x, %02x, %02x, %02x)...",
			buf[o2 + 0], buf[o2 + 1], buf[o2 + 2], buf[o2 + 3], 
			buf[o2 + 4], buf[o2 + 5], buf[o2 + 6], buf[o2 + 7]);
		/* 00 1b is # of entries in Motorola order - the rest should also be in MM order */
		n->order = EXIF_BYTE_ORDER_MOTOROLA;
		break;

	default:
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataOlympus",
			"Unknown Olympus variant %i.", n->version);
		return;
	}

	/* Sanity check the offset */
	if (CHECKOVERFLOW(o2,buf_size,2)) {
		exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
			  "ExifMnoteOlympus", "Short MakerNote");
		return;
	}

	/* Read the number of tags */
	c = exif_get_short (buf + o2, n->order);
	o2 += 2;

	/* Remove any old entries */
	exif_mnote_data_olympus_clear (n);

	/* Reserve enough space for all the possible MakerNote tags */
	n->entries = exif_mem_alloc (en->mem, sizeof (MnoteOlympusEntry) * c);
	if (!n->entries) {
		EXIF_LOG_NO_MEMORY(en->log, "ExifMnoteOlympus", sizeof (MnoteOlympusEntry) * c);
		return;
	}

	/* Parse all c entries, storing ones that are successfully parsed */
	tcount = 0;
	for (i = c, o = o2; i; --i, o += 12) {
		size_t s;
		if (CHECKOVERFLOW(o, buf_size, 12)) {
			exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
				  "ExifMnoteOlympus", "Short MakerNote");
			break;
		}

	    n->entries[tcount].tag        = exif_get_short (buf + o, n->order) + base;
	    n->entries[tcount].format     = exif_get_short (buf + o + 2, n->order);
	    n->entries[tcount].components = exif_get_long (buf + o + 4, n->order);
	    n->entries[tcount].order      = n->order;

	    exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteOlympus",
		      "Loading entry 0x%x ('%s')...", n->entries[tcount].tag,
		      mnote_olympus_tag_get_name (n->entries[tcount].tag));
/*	    exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteOlympus",
			    "0x%x %d %ld*(%d)",
		    n->entries[tcount].tag,
		    n->entries[tcount].format,
		    n->entries[tcount].components,
		    (int)exif_format_get_size(n->entries[tcount].format)); */

	    /* Check if we overflow the multiplication. Use buf_size as the max size for integer overflow detection,
	     * we will check the buffer sizes closer later. */
	    if (exif_format_get_size (n->entries[tcount].format) &&
		buf_size / exif_format_get_size (n->entries[tcount].format) < n->entries[tcount].components
	    ) {
		exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA, "ExifMnoteOlympus", "Tag size overflow detected (%u * %lu)", exif_format_get_size (n->entries[tcount].format), n->entries[tcount].components);
		continue;
	    }
	    /*
	     * Size? If bigger than 4 bytes, the actual data is not
	     * in the entry but somewhere else (offset).
	     */
	    s = exif_format_get_size (n->entries[tcount].format) *
		   			 n->entries[tcount].components;
		n->entries[tcount].size = s;
		if (s) {
			size_t dataofs = o + 8;
			if (s > 4) {
				/* The data in this case is merely a pointer */
				dataofs = exif_get_long (buf + dataofs, n->order) + datao;
#ifdef EXIF_OVERCOME_SANYO_OFFSET_BUG
				/* Some Sanyo models (e.g. VPC-C5, C40) suffer from a bug when
				 * writing the offset for the MNOTE_OLYMPUS_TAG_THUMBNAILIMAGE
				 * tag in its MakerNote. The offset is actually the absolute
				 * position in the file instead of the position within the IFD.
				 */
			    if (dataofs > (buf_size - s) && n->version == sanyoV1) {
					/* fix pointer */
					dataofs -= datao + 6;
					exif_log (en->log, EXIF_LOG_CODE_DEBUG,
						  "ExifMnoteOlympus",
						  "Inconsistent thumbnail tag offset; attempting to recover");
			    }
#endif
			}
			if (CHECKOVERFLOW(dataofs, buf_size, s)) {
				exif_log (en->log, EXIF_LOG_CODE_DEBUG,
					  "ExifMnoteOlympus",
					  "Tag data past end of buffer (%u > %u)",
					  (unsigned)(dataofs + s), buf_size);
				continue;
			}

			n->entries[tcount].data = exif_mem_alloc (en->mem, s);
			if (!n->entries[tcount].data) {
				EXIF_LOG_NO_MEMORY(en->log, "ExifMnoteOlympus", s);
				continue;
			}
			memcpy (n->entries[tcount].data, buf + dataofs, s);
		}

		/* Tag was successfully parsed */
		++tcount;
	}
	/* Store the count of successfully parsed tags */
	n->count = tcount;
}