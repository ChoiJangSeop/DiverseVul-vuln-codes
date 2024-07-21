static char* set_iovec_field(struct iovec *iovec, size_t *n_iovec, const char *field, const char *value) {
        char *x;

        x = strappend(field, value);
        if (x)
                iovec[(*n_iovec)++] = IOVEC_MAKE_STRING(x);
        return x;
}