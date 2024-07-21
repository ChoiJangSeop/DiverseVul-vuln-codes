dir_globs(long argc, const VALUE *argv, VALUE base, int flags)
{
    VALUE ary = rb_ary_new();
    long i;

    for (i = 0; i < argc; ++i) {
	int status;
	VALUE str = argv[i];
	GlobPathValue(str, TRUE);
	status = push_glob(ary, str, base, flags);
	if (status) GLOB_JUMP_TAG(status);
    }

    return ary;
}