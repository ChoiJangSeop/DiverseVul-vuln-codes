size_t c_shquote_strnspn(const char *string,
                        size_t n_string,
                        const char *accept) {
        bool buffer[UCHAR_MAX] = {};

        for ( ; *accept; ++accept)
                buffer[(unsigned char)*accept] = true;

        for (size_t i = 0; i < n_string; ++i)
                if (!buffer[(unsigned char)string[i]])
                        return i;

        return n_string;
}