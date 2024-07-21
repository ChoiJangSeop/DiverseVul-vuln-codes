lib_set_system_encoding(self, enc_name)
    VALUE self;
    VALUE enc_name;
{
#if TCL_MAJOR_VERSION > 8 || (TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION > 0)
    tcl_stubs_check();

    if (NIL_P(enc_name)) {
        Tcl_SetSystemEncoding((Tcl_Interp *)NULL, (CONST char *)NULL);
        return lib_get_system_encoding(self);
    }

    enc_name = rb_funcallv(enc_name, ID_to_s, 0, 0);
    if (Tcl_SetSystemEncoding((Tcl_Interp *)NULL,
                              StringValuePtr(enc_name)) != TCL_OK) {
        rb_raise(rb_eArgError, "unknown encoding name '%s'",
                 RSTRING_PTR(enc_name));
    }

    return enc_name;
#else
    return Qnil;
#endif
}