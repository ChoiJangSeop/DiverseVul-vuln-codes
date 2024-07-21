key_set(const char *key) {
    int		len = (int)strlen(key);
    int64_t	h = calc_hash(key, &len);
    Slot	*bucket = get_bucketp(h);
    Slot	s;
    
    if (NULL != (s = (Slot)AGOO_MALLOC(sizeof(struct _slot)))) {
	s->hash = h;
	s->klen = len;
	s->key = key;
	s->next = *bucket;
	*bucket = s;
    }
}