rb_dir_s_empty_p(VALUE obj, VALUE dirname)
{
    VALUE result, orig;
    const char *path;
    enum {false_on_notdir = 1};

    GlobPathValue(dirname, FALSE);
    orig = rb_str_dup_frozen(dirname);
    dirname = rb_str_encode_ospath(dirname);
    dirname = rb_str_dup_frozen(dirname);
    path = RSTRING_PTR(dirname);

#if defined HAVE_GETATTRLIST && defined ATTR_DIR_ENTRYCOUNT
    {
	u_int32_t attrbuf[SIZEUP32(fsobj_tag_t)];
	struct attrlist al = {ATTR_BIT_MAP_COUNT, 0, ATTR_CMN_OBJTAG,};
	if (getattrlist(path, &al, attrbuf, sizeof(attrbuf), 0) != 0)
	    rb_sys_fail_path(orig);
	if (*(const fsobj_tag_t *)(attrbuf+1) == VT_HFS) {
	    al.commonattr = 0;
	    al.dirattr = ATTR_DIR_ENTRYCOUNT;
	    if (getattrlist(path, &al, attrbuf, sizeof(attrbuf), 0) == 0) {
		if (attrbuf[0] >= 2 * sizeof(u_int32_t))
		    return attrbuf[1] ? Qfalse : Qtrue;
		if (false_on_notdir) return Qfalse;
	    }
	    rb_sys_fail_path(orig);
	}
    }
#endif

    result = (VALUE)rb_thread_call_without_gvl(nogvl_dir_empty_p, (void *)path,
					    RUBY_UBF_IO, 0);
    if (result == Qundef) {
	rb_sys_fail_path(orig);
    }
    return result;
}