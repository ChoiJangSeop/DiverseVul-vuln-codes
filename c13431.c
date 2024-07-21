agoo_http_init() {
    const char	**kp = header_keys;
    
    memset(&key_cache, 0, sizeof(struct _cache));
    for (; NULL != *kp; kp++) {
	key_set(*kp);
    }
}