dir_glob_options(VALUE opt, VALUE *base, int *flags)
{
    ID kw[2];
    VALUE args[2];
    kw[0] = rb_intern("base");
    if (flags) kw[1] = rb_intern("flags");
    rb_get_kwargs(opt, kw, 0, flags ? 2 : 1, args);
    if (args[0] == Qundef || NIL_P(args[0])) {
	*base = Qnil;
    }
#if USE_OPENDIR_AT
    else if (rb_typeddata_is_kind_of(args[0], &dir_data_type)) {
	*base = args[0];
    }
#endif
    else {
	GlobPathValue(args[0], TRUE);
	if (!RSTRING_LEN(args[0])) args[0] = Qnil;
	*base = args[0];
    }
    if (flags && args[1] != Qundef) {
	*flags = NUM2INT(args[1]);
    }
}