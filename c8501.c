exif_data_load_data_entry (ExifData *data, ExifEntry *entry,
			   const unsigned char *d,
			   unsigned int size, unsigned int offset)
{
	unsigned int s, doff;

	entry->tag        = exif_get_short (d + offset + 0, data->priv->order);
	entry->format     = exif_get_short (d + offset + 2, data->priv->order);
	entry->components = exif_get_long  (d + offset + 4, data->priv->order);

	/* FIXME: should use exif_tag_get_name_in_ifd here but entry->parent 
	 * has not been set yet
	 */
	exif_log (data->priv->log, EXIF_LOG_CODE_DEBUG, "ExifData",
		  "Loading entry 0x%x ('%s')...", entry->tag,
		  exif_tag_get_name (entry->tag));

	/* {0,1,2,4,8} x { 0x00000000 .. 0xffffffff } 
	 *   -> { 0x000000000 .. 0x7fffffff8 } */
	s = exif_format_get_size(entry->format) * entry->components;
	if ((s < entry->components) || (s == 0)){
		return 0;
	}

	/*
	 * Size? If bigger than 4 bytes, the actual data is not
	 * in the entry but somewhere else (offset).
	 */
	if (s > 4)
		doff = exif_get_long (d + offset + 8, data->priv->order);
	else
		doff = offset + 8;

	/* Sanity checks */
	if ((doff + s < doff) || (doff + s < s) || (doff + s > size)) {
		exif_log (data->priv->log, EXIF_LOG_CODE_DEBUG, "ExifData",
				  "Tag data past end of buffer (%u > %u)", doff+s, size);	
		return 0;
	}

	entry->data = exif_data_alloc (data, s);
	if (entry->data) {
		entry->size = s;
		memcpy (entry->data, d + doff, s);
	} else {
		EXIF_LOG_NO_MEMORY(data->priv->log, "ExifData", s);
		return 0;
	}

	/* If this is the MakerNote, remember the offset */
	if (entry->tag == EXIF_TAG_MAKER_NOTE) {
		if (!entry->data) {
			exif_log (data->priv->log, EXIF_LOG_CODE_DEBUG, "ExifData",
					  "MakerNote found with empty data");	
		} else if (entry->size > 6) {
			exif_log (data->priv->log,
					       EXIF_LOG_CODE_DEBUG, "ExifData",
					       "MakerNote found (%02x %02x %02x %02x "
					       "%02x %02x %02x...).",
					       entry->data[0], entry->data[1], entry->data[2],
					       entry->data[3], entry->data[4], entry->data[5],
					       entry->data[6]);
		}
		data->priv->offset_mnote = doff;
	}
	return 1;
}