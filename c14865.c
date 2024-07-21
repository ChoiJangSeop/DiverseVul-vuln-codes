int unit_name_path_escape(const char *f, char **ret) {
        char *p, *s;

        assert(f);
        assert(ret);

        p = strdupa(f);
        if (!p)
                return -ENOMEM;

        path_simplify(p, false);

        if (empty_or_root(p))
                s = strdup("-");
        else {
                if (!path_is_normalized(p))
                        return -EINVAL;

                /* Truncate trailing slashes */
                delete_trailing_chars(p, "/");

                /* Truncate leading slashes */
                p = skip_leading_chars(p, "/");

                s = unit_name_escape(p);
        }
        if (!s)
                return -ENOMEM;

        *ret = s;
        return 0;
}