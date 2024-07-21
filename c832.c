cdf_read_long_sector_chain(const cdf_info_t *info, const cdf_header_t *h,
    const cdf_sat_t *sat, cdf_secid_t sid, size_t len, cdf_stream_t *scn)
{
	size_t ss = CDF_SEC_SIZE(h), i, j;
	ssize_t nr;
	scn->sst_len = cdf_count_chain(sat, sid, ss);
	scn->sst_dirlen = len;

	if (scn->sst_len == (size_t)-1)
		return -1;

	scn->sst_tab = calloc(scn->sst_len, ss);
	if (scn->sst_tab == NULL)
		return -1;

	for (j = i = 0; sid >= 0; i++, j++) {
		if (j >= CDF_LOOP_LIMIT) {
			DPRINTF(("Read long sector chain loop limit"));
			errno = EFTYPE;
			goto out;
		}
		if (i >= scn->sst_len) {
			DPRINTF(("Out of bounds reading long sector chain "
			    "%u > %u\n", i, scn->sst_len));
			errno = EFTYPE;
			goto out;
		}
		if ((nr = cdf_read_sector(info, scn->sst_tab, i * ss, ss, h,
		    sid)) != (ssize_t)ss) {
			if (i == scn->sst_len - 1 && nr > 0) {
				/* Last sector might be truncated */
				return 0;
			}
			DPRINTF(("Reading long sector chain %d", sid));
			goto out;
		}
		sid = CDF_TOLE4((uint32_t)sat->sat_tab[sid]);
	}
	return 0;
out:
	free(scn->sst_tab);
	return -1;
}