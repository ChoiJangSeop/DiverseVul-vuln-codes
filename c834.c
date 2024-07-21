cdf_read_ssat(const cdf_info_t *info, const cdf_header_t *h,
    const cdf_sat_t *sat, cdf_sat_t *ssat)
{
	size_t i, j;
	size_t ss = CDF_SEC_SIZE(h);
	cdf_secid_t sid = h->h_secid_first_sector_in_short_sat;

	ssat->sat_len = cdf_count_chain(sat, sid, CDF_SEC_SIZE(h));
	if (ssat->sat_len == (size_t)-1)
		return -1;

	ssat->sat_tab = CAST(cdf_secid_t *, calloc(ssat->sat_len, ss));
	if (ssat->sat_tab == NULL)
		return -1;

	for (j = i = 0; sid >= 0; i++, j++) {
		if (j >= CDF_LOOP_LIMIT) {
			DPRINTF(("Read short sat sector loop limit"));
			errno = EFTYPE;
			goto out;
		}
		if (i >= ssat->sat_len) {
			DPRINTF(("Out of bounds reading short sector chain "
			    "%u > %u\n", i, ssat->sat_len));
			errno = EFTYPE;
			goto out;
		}
		if (cdf_read_sector(info, ssat->sat_tab, i * ss, ss, h, sid) !=
		    (ssize_t)ss) {
			DPRINTF(("Reading short sat sector %d", sid));
			goto out;
		}
		sid = CDF_TOLE4((uint32_t)sat->sat_tab[sid]);
	}
	return 0;
out:
	free(ssat->sat_tab);
	return -1;
}