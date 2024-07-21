date_s__parse_internal(int argc, VALUE *argv, VALUE klass)
{
    VALUE vstr, vcomp, hash;

    rb_scan_args(argc, argv, "11", &vstr, &vcomp);
    StringValue(vstr);
    if (!rb_enc_str_asciicompat_p(vstr))
	rb_raise(rb_eArgError,
		 "string should have ASCII compatible encoding");
    if (argc < 2)
	vcomp = Qtrue;

    hash = date__parse(vstr, vcomp);

    return hash;
}