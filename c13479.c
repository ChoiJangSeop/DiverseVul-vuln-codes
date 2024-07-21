size_t c_shquote_strncspn(const char *string,
                          size_t n_string,
                          const char *reject) {
        bool buffer[UCHAR_MAX] = {};

        if (strlen(reject) == 1) {
                const char *p;

                p = memchr(string, reject[0], n_string);
                if (!p)
                        return n_string;
                else
                        return p - string;
        }

        for ( ; *reject; ++reject)
                buffer[(unsigned char)*reject] = true;

        for (size_t i = 0; i < n_string; ++i)
                if (buffer[(unsigned char)string[i]])
                        return i;

        return n_string;
}