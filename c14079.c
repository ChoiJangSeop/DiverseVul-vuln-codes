char *ad_get_entry(const struct adouble *ad, int eid)
{
	off_t off = ad_getentryoff(ad, eid);
	size_t len = ad_getentrylen(ad, eid);

	if (off == 0 || len == 0) {
		return NULL;
	}

	return ad->ad_data + off;
}