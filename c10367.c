copy_rrset(const struct ub_packed_rrset_key* key, struct regional* region)
{
	struct ub_packed_rrset_key* ck = regional_alloc(region,
		sizeof(struct ub_packed_rrset_key));
	struct packed_rrset_data* d;
	struct packed_rrset_data* data = key->entry.data;
	size_t dsize, i;
	uint8_t* nextrdata;

	/* derived from packed_rrset_copy_region(), but don't use
	 * packed_rrset_sizeof() and do exclude RRSIGs */
	if(!ck)
		return NULL;
	ck->id = key->id;
	memset(&ck->entry, 0, sizeof(ck->entry));
	ck->entry.hash = key->entry.hash;
	ck->entry.key = ck;
	ck->rk = key->rk;
	ck->rk.dname = regional_alloc_init(region, key->rk.dname,
		key->rk.dname_len);
	if(!ck->rk.dname)
		return NULL;

	dsize = sizeof(struct packed_rrset_data) + data->count *
		(sizeof(size_t)+sizeof(uint8_t*)+sizeof(time_t));
	for(i=0; i<data->count; i++)
		dsize += data->rr_len[i];
	d = regional_alloc(region, dsize);
	if(!d)
		return NULL;
	*d = *data;
	d->rrsig_count = 0;
	ck->entry.data = d;

	/* derived from packed_rrset_ptr_fixup() with copying the data */
	d->rr_len = (size_t*)((uint8_t*)d + sizeof(struct packed_rrset_data));
	d->rr_data = (uint8_t**)&(d->rr_len[d->count]);
	d->rr_ttl = (time_t*)&(d->rr_data[d->count]);
	nextrdata = (uint8_t*)&(d->rr_ttl[d->count]);
	for(i=0; i<d->count; i++) {
		d->rr_len[i] = data->rr_len[i];
		d->rr_ttl[i] = data->rr_ttl[i];
		d->rr_data[i] = nextrdata;
		memcpy(d->rr_data[i], data->rr_data[i], data->rr_len[i]);
		nextrdata += d->rr_len[i];
	}

	return ck;
}