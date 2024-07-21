static VALUE cState_space_set(VALUE self, VALUE space)
{
    unsigned long len;
    GET_STATE(self);
    Check_Type(space, T_STRING);
    len = RSTRING_LEN(space);
    if (len == 0) {
        if (state->space) {
            ruby_xfree(state->space);
            state->space = NULL;
            state->space_len = 0;
        }
    } else {
        if (state->space) ruby_xfree(state->space);
        state->space = strdup(RSTRING_PTR(space));
        state->space_len = len;
    }
    return Qnil;
}