exif_data_load_data (ExifData *data, const unsigned char *d_orig,
		     unsigned int ds)
{
	unsigned int l;
	ExifLong offset;
	ExifShort n;
	const unsigned char *d = d_orig;
	unsigned int len, fullds;

	if (!data || !data->priv || !d || !ds)
		return;

	exif_log (data->priv->log, EXIF_LOG_CODE_DEBUG, "ExifData",
		  "Parsing %i byte(s) EXIF data...\n", ds);

	/*
	 * It can be that the data starts with the EXIF header. If it does
	 * not, search the EXIF marker.
	 */
	if (ds < 6) {
		LOG_TOO_SMALL;
		return;
	}
	if (!memcmp (d, ExifHeader, 6)) {
		exif_log (data->priv->log, EXIF_LOG_CODE_DEBUG, "ExifData",
			  "Found EXIF header at start.");
	} else {
		while (ds >= 3) {
			while (ds && (d[0] == 0xff)) {
				d++;
				ds--;
			}

			/* JPEG_MARKER_SOI */
			if (ds && d[0] == JPEG_MARKER_SOI) {
				d++;
				ds--;
				continue;
			}

			/* JPEG_MARKER_APP1 */
			if (ds && d[0] == JPEG_MARKER_APP1)
				break;

			/* Skip irrelevant APP markers. The branch for APP1 must come before this,
			   otherwise this code block will cause APP1 to be skipped. This code path
			   is only relevant for files that are nonconformant to the EXIF
			   specification. For conformant files, the APP1 code path above will be
			   taken. */
			if (ds >= 3 && d[0] >= 0xe0 && d[0] <= 0xef) {  /* JPEG_MARKER_APPn */
				d++;
				ds--;
				l = (d[0] << 8) | d[1];
				if (l > ds)
					return;
				d += l;
				ds -= l;
				continue;
			}

			/* Unknown marker or data. Give up. */
			exif_log (data->priv->log, EXIF_LOG_CODE_CORRUPT_DATA,
				  "ExifData", _("EXIF marker not found."));
			return;
		}
		if (ds < 3) {
			LOG_TOO_SMALL;
			return;
		}
		d++;
		ds--;
		len = (d[0] << 8) | d[1];
		exif_log (data->priv->log, EXIF_LOG_CODE_DEBUG, "ExifData",
			  "We have to deal with %i byte(s) of EXIF data.",
			  len);
		d += 2;
		ds -= 2;
	}

	/*
	 * Verify the exif header
	 * (offset 2, length 6).
	 */
	if (ds < 6) {
		LOG_TOO_SMALL;
		return;
	}
	if (memcmp (d, ExifHeader, 6)) {
		exif_log (data->priv->log, EXIF_LOG_CODE_CORRUPT_DATA,
			  "ExifData", _("EXIF header not found."));
		return;
	}

	exif_log (data->priv->log, EXIF_LOG_CODE_DEBUG, "ExifData",
		  "Found EXIF header.");

	/* Sanity check the data length */
	if (ds < 14)
		return;

	/* The JPEG APP1 section can be no longer than 64 KiB (including a
	   16-bit length), so cap the data length to protect against overflow
	   in future offset calculations */
	fullds = ds;
	if (ds > 0xfffe)
		ds = 0xfffe;

	/* Byte order (offset 6, length 2) */
	if (!memcmp (d + 6, "II", 2))
		data->priv->order = EXIF_BYTE_ORDER_INTEL;
	else if (!memcmp (d + 6, "MM", 2))
		data->priv->order = EXIF_BYTE_ORDER_MOTOROLA;
	else {
		exif_log (data->priv->log, EXIF_LOG_CODE_CORRUPT_DATA,
			  "ExifData", _("Unknown encoding."));
		return;
	}

	/* Fixed value */
	if (exif_get_short (d + 8, data->priv->order) != 0x002a)
		return;

	/* IFD 0 offset */
	offset = exif_get_long (d + 10, data->priv->order);
	exif_log (data->priv->log, EXIF_LOG_CODE_DEBUG, "ExifData", 
		  "IFD 0 at %i.", (int) offset);

	/* Sanity check the offset, being careful about overflow */
	if (offset > ds || offset + 6 + 2 > ds)
		return;

	/* Parse the actual exif data (usually offset 14 from start) */
	exif_data_load_data_content (data, EXIF_IFD_0, d + 6, ds - 6, offset, 0);

	/* IFD 1 offset */
	n = exif_get_short (d + 6 + offset, data->priv->order);
	if (offset + 6 + 2 + 12 * n + 4 > ds)
		return;

	offset = exif_get_long (d + 6 + offset + 2 + 12 * n, data->priv->order);
	if (offset) {
		exif_log (data->priv->log, EXIF_LOG_CODE_DEBUG, "ExifData",
			  "IFD 1 at %i.", (int) offset);

		/* Sanity check. */
		if (offset > ds || offset + 6 > ds) {
			exif_log (data->priv->log, EXIF_LOG_CODE_CORRUPT_DATA,
				  "ExifData", "Bogus offset of IFD1.");
		} else {
		   exif_data_load_data_content (data, EXIF_IFD_1, d + 6, ds - 6, offset, 0);
		}
	}

	/*
	 * If we got an EXIF_TAG_MAKER_NOTE, try to interpret it. Some
	 * cameras use pointers in the maker note tag that point to the
	 * space between IFDs. Here is the only place where we have access
	 * to that data.
	 */
	interpret_maker_note(data, d, fullds);

	/* Fixup tags if requested */
	if (data->priv->options & EXIF_DATA_OPTION_FOLLOW_SPECIFICATION)
		exif_data_fix (data);
}