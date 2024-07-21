static bool ad_unpack_xattrs(struct adouble *ad)
{
	struct ad_xattr_header *h = &ad->adx_header;
	const char *p = ad->ad_data;
	uint32_t hoff;
	uint32_t i;

	if (ad_getentrylen(ad, ADEID_FINDERI) <= ADEDLEN_FINDERI) {
		return true;
	}

	/* 2 bytes padding */
	hoff = ad_getentryoff(ad, ADEID_FINDERI) + ADEDLEN_FINDERI + 2;

	h->adx_magic       = RIVAL(p, hoff + 0);
	h->adx_debug_tag   = RIVAL(p, hoff + 4); /* Not used -> not checked */
	h->adx_total_size  = RIVAL(p, hoff + 8);
	h->adx_data_start  = RIVAL(p, hoff + 12);
	h->adx_data_length = RIVAL(p, hoff + 16);
	h->adx_flags       = RSVAL(p, hoff + 32); /* Not used -> not checked */
	h->adx_num_attrs   = RSVAL(p, hoff + 34);

	if (h->adx_magic != AD_XATTR_HDR_MAGIC) {
		DBG_ERR("Bad magic: 0x%" PRIx32 "\n", h->adx_magic);
		return false;
	}

	if (h->adx_total_size > ad_getentryoff(ad, ADEID_RFORK)) {
		DBG_ERR("Bad total size: 0x%" PRIx32 "\n", h->adx_total_size);
		return false;
	}
	if (h->adx_total_size > AD_XATTR_MAX_HDR_SIZE) {
		DBG_ERR("Bad total size: 0x%" PRIx32 "\n", h->adx_total_size);
		return false;
	}

	if (h->adx_data_start < (hoff + AD_XATTR_HDR_SIZE)) {
		DBG_ERR("Bad start: 0x%" PRIx32 "\n", h->adx_data_start);
		return false;
	}

	if ((h->adx_data_start + h->adx_data_length) < h->adx_data_start) {
		DBG_ERR("Bad length: %" PRIu32 "\n", h->adx_data_length);
		return false;
	}
	if ((h->adx_data_start + h->adx_data_length) >
	    ad->adx_header.adx_total_size)
	{
		DBG_ERR("Bad length: %" PRIu32 "\n", h->adx_data_length);
		return false;
	}

	if (h->adx_num_attrs > AD_XATTR_MAX_ENTRIES) {
		DBG_ERR("Bad num xattrs: %" PRIu16 "\n", h->adx_num_attrs);
		return false;
	}

	if (h->adx_num_attrs == 0) {
		return true;
	}

	ad->adx_entries = talloc_zero_array(
		ad, struct ad_xattr_entry, h->adx_num_attrs);
	if (ad->adx_entries == NULL) {
		return false;
	}

	hoff += AD_XATTR_HDR_SIZE;

	for (i = 0; i < h->adx_num_attrs; i++) {
		struct ad_xattr_entry *e = &ad->adx_entries[i];

		hoff = (hoff + 3) & ~3;

		e->adx_offset  = RIVAL(p, hoff + 0);
		e->adx_length  = RIVAL(p, hoff + 4);
		e->adx_flags   = RSVAL(p, hoff + 8);
		e->adx_namelen = *(p + hoff + 10);

		if (e->adx_offset >= ad->adx_header.adx_total_size) {
			DBG_ERR("Bad adx_offset: %" PRIx32 "\n",
				e->adx_offset);
			return false;
		}

		if ((e->adx_offset + e->adx_length) < e->adx_offset) {
			DBG_ERR("Bad adx_length: %" PRIx32 "\n",
				e->adx_length);
			return false;
		}

		if ((e->adx_offset + e->adx_length) >
		    ad->adx_header.adx_total_size)
		{
			DBG_ERR("Bad adx_length: %" PRIx32 "\n",
				e->adx_length);
			return false;
		}

		if (e->adx_namelen == 0) {
			DBG_ERR("Bad adx_namelen: %" PRIx32 "\n",
				e->adx_namelen);
			return false;
		}
		if ((hoff + 11 + e->adx_namelen) < hoff + 11) {
			DBG_ERR("Bad adx_namelen: %" PRIx32 "\n",
				e->adx_namelen);
			return false;
		}
		if ((hoff + 11 + e->adx_namelen) >
		    ad->adx_header.adx_data_start)
		{
			DBG_ERR("Bad adx_namelen: %" PRIx32 "\n",
				e->adx_namelen);
			return false;
		}

		e->adx_name = talloc_strndup(ad->adx_entries,
					     p + hoff + 11,
					     e->adx_namelen);
		if (e->adx_name == NULL) {
			return false;
		}

		DBG_DEBUG("xattr [%s] offset [0x%x] size [0x%x]\n",
			  e->adx_name, e->adx_offset, e->adx_length);
		dump_data(10, (uint8_t *)(ad->ad_data + e->adx_offset),
			  e->adx_length);

		hoff += 11 + e->adx_namelen;
	}

	return true;
}