static VALUE cState_space_before_set(VALUE self, VALUE space_before)
{
    unsigned long len;
    GET_STATE(self);
    Check_Type(space_before, T_STRING);
    len = RSTRING_LEN(space_before);
    if (len == 0) {
        if (state->space_before) {
            ruby_xfree(state->space_before);
            state->space_before = NULL;
            state->space_before_len = 0;
        }
    } else {
        if (state->space_before) ruby_xfree(state->space_before);
        state->space_before = strdup(RSTRING_PTR(space_before));
        state->space_before_len = len;
    }
    return Qnil;
}