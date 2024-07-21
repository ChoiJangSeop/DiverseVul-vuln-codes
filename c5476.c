static VALUE cState_array_nl_set(VALUE self, VALUE array_nl)
{
    unsigned long len;
    GET_STATE(self);
    Check_Type(array_nl, T_STRING);
    len = RSTRING_LEN(array_nl);
    if (len == 0) {
        if (state->array_nl) {
            ruby_xfree(state->array_nl);
            state->array_nl = NULL;
        }
    } else {
        if (state->array_nl) ruby_xfree(state->array_nl);
        state->array_nl = strdup(RSTRING_PTR(array_nl));
        state->array_nl_len = len;
    }
    return Qnil;
}