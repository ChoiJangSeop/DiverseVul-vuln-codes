add_header_value(VALUE hh, const char *key, int klen, const char *val, int vlen) {
    if (sizeof(content_type) - 1 == klen && 0 == strncasecmp(key, content_type, sizeof(content_type) - 1)) {
	rb_hash_aset(hh, content_type_val, rb_str_new(val, vlen));
    } else if (sizeof(content_length) - 1 == klen && 0 == strncasecmp(key, content_length, sizeof(content_length) - 1)) {
	rb_hash_aset(hh, content_length_val, rb_str_new(val, vlen));
    } else {
	char		hkey[1024];
	char		*k = hkey;
	volatile VALUE	sval = rb_str_new(val, vlen);

	strcpy(hkey, "HTTP_");
	k = hkey + 5;
	if ((int)(sizeof(hkey) - 5) <= klen) {
	    klen = sizeof(hkey) - 6;
	}
	strncpy(k, key, klen);
	hkey[klen + 5] = '\0';

	//rb_hash_aset(hh, rb_str_new(hkey, klen + 5), sval);
	// Contrary to the Rack spec, Rails expects all upper case keys so add those as well.
	for (k = hkey + 5; '\0' != *k; k++) {
	    if ('-' == *k) {
		*k = '_';
	    } else {
		*k = toupper(*k);
	    }
	}
	rb_hash_aset(hh, rb_str_new(hkey, klen + 5), sval);
    }
}