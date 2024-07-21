rb_fiddle_handle_s_sym(VALUE self, VALUE sym)
{
    return fiddle_handle_sym(RTLD_NEXT, StringValueCStr(sym));
}