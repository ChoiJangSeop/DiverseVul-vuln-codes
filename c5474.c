static VALUE cState_indent_set(VALUE self, VALUE indent)
{
    unsigned long len;
    GET_STATE(self);
    Check_Type(indent, T_STRING);
    len = RSTRING_LEN(indent);
    if (len == 0) {
        if (state->indent) {
            ruby_xfree(state->indent);
            state->indent = NULL;
            state->indent_len = 0;
        }
    } else {
        if (state->indent) ruby_xfree(state->indent);
        state->indent = strdup(RSTRING_PTR(indent));
        state->indent_len = len;
    }
    return Qnil;
}