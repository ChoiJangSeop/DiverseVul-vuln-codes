exif_mnote_data_pentax_load (ExifMnoteData *en,
		const unsigned char *buf, unsigned int buf_size)
{
	ExifMnoteDataPentax *n = (ExifMnoteDataPentax *) en;
	size_t i, tcount, o, datao, base = 0;
	ExifShort c;

	if (!n || !buf || !buf_size) {
		exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
			  "ExifMnoteDataPentax", "Short MakerNote");
		return;
	}
	datao = 6 + n->offset;
	if (CHECKOVERFLOW(datao, buf_size, 8)) {
		exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
			  "ExifMnoteDataPentax", "Short MakerNote");
		return;
	}

	/* Detect variant of Pentax/Casio MakerNote found */
	if (!memcmp(buf + datao, "AOC", 4)) {
		if ((buf[datao + 4] == 'I') && (buf[datao + 5] == 'I')) {
			n->version = pentaxV3;
			n->order = EXIF_BYTE_ORDER_INTEL;
		} else if ((buf[datao + 4] == 'M') && (buf[datao + 5] == 'M')) {
			n->version = pentaxV3;
			n->order = EXIF_BYTE_ORDER_MOTOROLA;
		} else {
			/* Uses Casio v2 tags */
			n->version = pentaxV2;
		}
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataPentax",
			"Parsing Pentax maker note v%d...", (int)n->version);
		datao += 4 + 2;
		base = MNOTE_PENTAX2_TAG_BASE;
	} else if (!memcmp(buf + datao, "QVC", 4)) {
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataPentax",
			"Parsing Casio maker note v2...");
		n->version = casioV2;
		base = MNOTE_CASIO2_TAG_BASE;
		datao += 4 + 2;
	} else {
		/* probably assert(!memcmp(buf + datao, "\x00\x1b", 2)) */
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataPentax",
			"Parsing Pentax maker note v1...");
		n->version = pentaxV1;
	}

	/* Read the number of tags */
	c = exif_get_short (buf + datao, n->order);
	datao += 2;

	/* Remove any old entries */
	exif_mnote_data_pentax_clear (n);

	/* Reserve enough space for all the possible MakerNote tags */
	n->entries = exif_mem_alloc (en->mem, sizeof (MnotePentaxEntry) * c);
	if (!n->entries) {
		EXIF_LOG_NO_MEMORY(en->log, "ExifMnoteDataPentax", sizeof (MnotePentaxEntry) * c);
		return;
	}

	/* Parse all c entries, storing ones that are successfully parsed */
	tcount = 0;
	for (i = c, o = datao; i; --i, o += 12) {
		size_t s;

		if (CHECKOVERFLOW(o,buf_size,12)) {
			exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
				  "ExifMnoteDataPentax", "Short MakerNote");
			break;
		}

		n->entries[tcount].tag        = exif_get_short (buf + o + 0, n->order) + base;
		n->entries[tcount].format     = exif_get_short (buf + o + 2, n->order);
		n->entries[tcount].components = exif_get_long  (buf + o + 4, n->order);
		n->entries[tcount].order      = n->order;

		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnotePentax",
			  "Loading entry 0x%x ('%s')...", n->entries[tcount].tag,
			  mnote_pentax_tag_get_name (n->entries[tcount].tag));

		/* Check if we overflow the multiplication. Use buf_size as the max size for integer overflow detection,
		 * we will check the buffer sizes closer later. */
		if (	exif_format_get_size (n->entries[tcount].format) &&
			buf_size / exif_format_get_size (n->entries[tcount].format) < n->entries[tcount].components
		) {
			exif_log (en->log, EXIF_LOG_CODE_CORRUPT_DATA,
				  "ExifMnoteDataPentax", "Tag size overflow detected (%u * %lu)", exif_format_get_size (n->entries[tcount].format), n->entries[tcount].components);
			break;
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
			if (s > 4)
				/* The data in this case is merely a pointer */
			   	dataofs = exif_get_long (buf + dataofs, n->order) + 6;

			if (CHECKOVERFLOW(dataofs, buf_size, s)) {
				exif_log (en->log, EXIF_LOG_CODE_DEBUG,
						  "ExifMnoteDataPentax", "Tag data past end "
					  "of buffer (%u > %u)", (unsigned)(dataofs + s), buf_size);
				continue;
			}

			n->entries[tcount].data = exif_mem_alloc (en->mem, s);
			if (!n->entries[tcount].data) {
				EXIF_LOG_NO_MEMORY(en->log, "ExifMnoteDataPentax", s);
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