file_s_fnmatch(int argc, VALUE *argv, VALUE obj)
{
    VALUE pattern, path;
    VALUE rflags;
    int flags;

    if (rb_scan_args(argc, argv, "21", &pattern, &path, &rflags) == 3)
	flags = NUM2INT(rflags);
    else
	flags = 0;

    StringValue(pattern);
    FilePathStringValue(path);

    if (flags & FNM_EXTGLOB) {
	struct brace_args args;

	args.value = path;
	args.flags = flags;
	if (ruby_brace_expand(RSTRING_PTR(pattern), flags, fnmatch_brace,
			      (VALUE)&args, rb_enc_get(pattern), pattern) > 0)
	    return Qtrue;
    }
    else {
	rb_encoding *enc = rb_enc_compatible(pattern, path);
	if (!enc) return Qfalse;
	if (fnmatch(RSTRING_PTR(pattern), enc, RSTRING_PTR(path), flags) == 0)
	    return Qtrue;
    }
    RB_GC_GUARD(pattern);

    return Qfalse;
}