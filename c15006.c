static bool ad_unpack(struct adouble *ad, const size_t nentries,
		      size_t filesize)
{
	size_t bufsize = talloc_get_size(ad->ad_data);
	size_t adentries, i;
	uint32_t eid, len, off;
	bool ok;

	/*
	 * The size of the buffer ad->ad_data is checked when read, so
	 * we wouldn't have to check our own offsets, a few extra
	 * checks won't hurt though. We have to check the offsets we
	 * read from the buffer anyway.
	 */

	if (bufsize < (AD_HEADER_LEN + (AD_ENTRY_LEN * nentries))) {
		DEBUG(1, ("bad size\n"));
		return false;
	}

	ad->ad_magic = RIVAL(ad->ad_data, 0);
	ad->ad_version = RIVAL(ad->ad_data, ADEDOFF_VERSION);
	if ((ad->ad_magic != AD_MAGIC) || (ad->ad_version != AD_VERSION)) {
		DEBUG(1, ("wrong magic or version\n"));
		return false;
	}

	memcpy(ad->ad_filler, ad->ad_data + ADEDOFF_FILLER, ADEDLEN_FILLER);

	adentries = RSVAL(ad->ad_data, ADEDOFF_NENTRIES);
	if (adentries != nentries) {
		DEBUG(1, ("invalid number of entries: %zu\n",
			  adentries));
		return false;
	}

	/* now, read in the entry bits */
	for (i = 0; i < adentries; i++) {
		eid = RIVAL(ad->ad_data, AD_HEADER_LEN + (i * AD_ENTRY_LEN));
		eid = get_eid(eid);
		off = RIVAL(ad->ad_data, AD_HEADER_LEN + (i * AD_ENTRY_LEN) + 4);
		len = RIVAL(ad->ad_data, AD_HEADER_LEN + (i * AD_ENTRY_LEN) + 8);

		if (!eid || eid >= ADEID_MAX) {
			DEBUG(1, ("bogus eid %d\n", eid));
			return false;
		}

		/*
		 * All entries other than the resource fork are
		 * expected to be read into the ad_data buffer, so
		 * ensure the specified offset is within that bound
		 */
		if ((off > bufsize) && (eid != ADEID_RFORK)) {
			DEBUG(1, ("bogus eid %d: off: %" PRIu32 ", len: %" PRIu32 "\n",
				  eid, off, len));
			return false;
		}

		/*
		 * All entries besides FinderInfo and resource fork
		 * must fit into the buffer. FinderInfo is special as
		 * it may be larger then the default 32 bytes (if it
		 * contains marshalled xattrs), but we will fixup that
		 * in ad_convert(). And the resource fork is never
		 * accessed directly by the ad_data buf (also see
		 * comment above) anyway.
		 */
		if ((eid != ADEID_RFORK) &&
		    (eid != ADEID_FINDERI) &&
		    ((off + len) > bufsize)) {
			DEBUG(1, ("bogus eid %d: off: %" PRIu32 ", len: %" PRIu32 "\n",
				  eid, off, len));
			return false;
		}

		/*
		 * That would be obviously broken
		 */
		if (off > filesize) {
			DEBUG(1, ("bogus eid %d: off: %" PRIu32 ", len: %" PRIu32 "\n",
				  eid, off, len));
			return false;
		}

		/*
		 * Check for any entry that has its end beyond the
		 * filesize.
		 */
		if (off + len < off) {
			DEBUG(1, ("offset wrap in eid %d: off: %" PRIu32
				  ", len: %" PRIu32 "\n",
				  eid, off, len));
			return false;

		}
		if (off + len > filesize) {
			/*
			 * If this is the resource fork entry, we fix
			 * up the length, for any other entry we bail
			 * out.
			 */
			if (eid != ADEID_RFORK) {
				DEBUG(1, ("bogus eid %d: off: %" PRIu32
					  ", len: %" PRIu32 "\n",
					  eid, off, len));
				return false;
			}

			/*
			 * Fixup the resource fork entry by limiting
			 * the size to entryoffset - filesize.
			 */
			len = filesize - off;
			DEBUG(1, ("Limiting ADEID_RFORK: off: %" PRIu32
				  ", len: %" PRIu32 "\n", off, len));
		}

		ad->ad_eid[eid].ade_off = off;
		ad->ad_eid[eid].ade_len = len;
	}

	ok = ad_unpack_xattrs(ad);
	if (!ok) {
		return false;
	}

	return true;
}