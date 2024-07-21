rsock_strbuf(VALUE str, long buflen)
{
    long len;

    if (NIL_P(str)) return rb_str_new(0, buflen);

    StringValue(str);
    len = RSTRING_LEN(str);
    if (len >= buflen) {
	rb_str_modify(str);
    } else {
	rb_str_modify_expand(str, buflen - len);
    }
    rb_str_set_len(str, buflen);
    return str;
}