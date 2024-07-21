size_t syslog_parse_identifier(const char **buf, char **identifier, char **pid) {
        const char *p;
        char *t;
        size_t l, e;

        assert(buf);
        assert(identifier);
        assert(pid);

        p = *buf;

        p += strspn(p, WHITESPACE);
        l = strcspn(p, WHITESPACE);

        if (l <= 0 ||
            p[l-1] != ':')
                return 0;

        e = l;
        l--;

        if (l > 0 && p[l-1] == ']') {
                size_t k = l-1;

                for (;;) {

                        if (p[k] == '[') {
                                t = strndup(p+k+1, l-k-2);
                                if (t)
                                        *pid = t;

                                l = k;
                                break;
                        }

                        if (k == 0)
                                break;

                        k--;
                }
        }

        t = strndup(p, l);
        if (t)
                *identifier = t;

        e += strspn(p + e, WHITESPACE);

        *buf = p + e;
        return e;
}