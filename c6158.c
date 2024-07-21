rb_push_glob(VALUE str, int flags) /* '\0' is delimiter */
{
    long offset = 0;
    VALUE ary;

    GlobPathValue(str, TRUE);
    ary = rb_ary_new();

    while (offset < RSTRING_LEN(str)) {
	char *p, *pend;
	int status;
	p = RSTRING_PTR(str) + offset;
	status = push_glob(ary, rb_enc_str_new(p, strlen(p), rb_enc_get(str)),
			   flags);
	if (status) GLOB_JUMP_TAG(status);
	if (offset >= RSTRING_LEN(str)) break;
	p += strlen(p) + 1;
	pend = RSTRING_PTR(str) + RSTRING_LEN(str);
	while (p < pend && !*p)
	    p++;
	offset = p - RSTRING_PTR(str);
    }

    return ary;
}