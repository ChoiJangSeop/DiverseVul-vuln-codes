cdf_read_sat(const cdf_info_t *info, cdf_header_t *h, cdf_sat_t *sat)
{
	size_t i, j, k;
	size_t ss = CDF_SEC_SIZE(h);
	cdf_secid_t *msa, mid, sec;
	size_t nsatpersec = (ss / sizeof(mid)) - 1;

	for (i = 0; i < __arraycount(h->h_master_sat); i++)
		if (h->h_master_sat[i] == CDF_SECID_FREE)
			break;

#define CDF_SEC_LIMIT (UINT32_MAX / (4 * ss))
	if ((nsatpersec > 0 &&
	    h->h_num_sectors_in_master_sat > CDF_SEC_LIMIT / nsatpersec) ||
	    i > CDF_SEC_LIMIT) {
		DPRINTF(("Number of sectors in master SAT too big %u %"
		    SIZE_T_FORMAT "u\n", h->h_num_sectors_in_master_sat, i));
		errno = EFTYPE;
		return -1;
	}

	sat->sat_len = h->h_num_sectors_in_master_sat * nsatpersec + i;
	DPRINTF(("sat_len = %" SIZE_T_FORMAT "u ss = %" SIZE_T_FORMAT "u\n",
	    sat->sat_len, ss));
	if ((sat->sat_tab = CAST(cdf_secid_t *, calloc(sat->sat_len, ss)))
	    == NULL)
		return -1;

	for (i = 0; i < __arraycount(h->h_master_sat); i++) {
		if (h->h_master_sat[i] < 0)
			break;
		if (cdf_read_sector(info, sat->sat_tab, ss * i, ss, h,
		    h->h_master_sat[i]) != (ssize_t)ss) {
			DPRINTF(("Reading sector %d", h->h_master_sat[i]));
			goto out1;
		}
	}

	if ((msa = CAST(cdf_secid_t *, calloc(1, ss))) == NULL)
		goto out1;

	mid = h->h_secid_first_sector_in_master_sat;
	for (j = 0; j < h->h_num_sectors_in_master_sat; j++) {
		if (mid < 0)
			goto out;
		if (j >= CDF_LOOP_LIMIT) {
			DPRINTF(("Reading master sector loop limit"));
			errno = EFTYPE;
			goto out2;
		}
		if (cdf_read_sector(info, msa, 0, ss, h, mid) != (ssize_t)ss) {
			DPRINTF(("Reading master sector %d", mid));
			goto out2;
		}
		for (k = 0; k < nsatpersec; k++, i++) {
			sec = CDF_TOLE4((uint32_t)msa[k]);
			if (sec < 0)
				goto out;
			if (i >= sat->sat_len) {
			    DPRINTF(("Out of bounds reading MSA %u >= %u",
				i, sat->sat_len));
			    errno = EFTYPE;
			    goto out2;
			}
			if (cdf_read_sector(info, sat->sat_tab, ss * i, ss, h,
			    sec) != (ssize_t)ss) {
				DPRINTF(("Reading sector %d",
				    CDF_TOLE4(msa[k])));
				goto out2;
			}
		}
		mid = CDF_TOLE4((uint32_t)msa[nsatpersec]);
	}
out:
	sat->sat_len = i;
	free(msa);
	return 0;
out2:
	free(msa);
out1:
	free(sat->sat_tab);
	return -1;
}