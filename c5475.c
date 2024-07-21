static VALUE cState_object_nl_set(VALUE self, VALUE object_nl)
{
    unsigned long len;
    GET_STATE(self);
    Check_Type(object_nl, T_STRING);
    len = RSTRING_LEN(object_nl);
    if (len == 0) {
        if (state->object_nl) {
            ruby_xfree(state->object_nl);
            state->object_nl = NULL;
        }
    } else {
        if (state->object_nl) ruby_xfree(state->object_nl);
        state->object_nl = strdup(RSTRING_PTR(object_nl));
        state->object_nl_len = len;
    }
    return Qnil;
}