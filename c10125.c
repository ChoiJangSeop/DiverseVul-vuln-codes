static void parse_xattrmap(struct lo_data *lo)
{
    const char *map = lo->xattrmap;
    const char *tmp;

    lo->xattr_map_nentries = 0;
    while (*map) {
        XattrMapEntry tmp_entry;
        char sep;

        if (isspace(*map)) {
            map++;
            continue;
        }
        /* The separator is the first non-space of the rule */
        sep = *map++;
        if (!sep) {
            break;
        }

        tmp_entry.flags = 0;
        /* Start of 'type' */
        if (strstart(map, "prefix", &map)) {
            tmp_entry.flags |= XATTR_MAP_FLAG_PREFIX;
        } else if (strstart(map, "ok", &map)) {
            tmp_entry.flags |= XATTR_MAP_FLAG_OK;
        } else if (strstart(map, "bad", &map)) {
            tmp_entry.flags |= XATTR_MAP_FLAG_BAD;
        } else if (strstart(map, "map", &map)) {
            /*
             * map is sugar that adds a number of rules, and must be
             * the last entry.
             */
            parse_xattrmap_map(lo, map, sep);
            return;
        } else {
            fuse_log(FUSE_LOG_ERR,
                     "%s: Unexpected type;"
                     "Expecting 'prefix', 'ok', 'bad' or 'map' in rule %zu\n",
                     __func__, lo->xattr_map_nentries);
            exit(1);
        }

        if (*map++ != sep) {
            fuse_log(FUSE_LOG_ERR,
                     "%s: Missing '%c' at end of type field of rule %zu\n",
                     __func__, sep, lo->xattr_map_nentries);
            exit(1);
        }

        /* Start of 'scope' */
        if (strstart(map, "client", &map)) {
            tmp_entry.flags |= XATTR_MAP_FLAG_CLIENT;
        } else if (strstart(map, "server", &map)) {
            tmp_entry.flags |= XATTR_MAP_FLAG_SERVER;
        } else if (strstart(map, "all", &map)) {
            tmp_entry.flags |= XATTR_MAP_FLAG_ALL;
        } else {
            fuse_log(FUSE_LOG_ERR,
                     "%s: Unexpected scope;"
                     " Expecting 'client', 'server', or 'all', in rule %zu\n",
                     __func__, lo->xattr_map_nentries);
            exit(1);
        }

        if (*map++ != sep) {
            fuse_log(FUSE_LOG_ERR,
                     "%s: Expecting '%c' found '%c'"
                     " after scope in rule %zu\n",
                     __func__, sep, *map, lo->xattr_map_nentries);
            exit(1);
        }

        /* At start of 'key' field */
        tmp = strchr(map, sep);
        if (!tmp) {
            fuse_log(FUSE_LOG_ERR,
                     "%s: Missing '%c' at end of key field of rule %zu",
                     __func__, sep, lo->xattr_map_nentries);
            exit(1);
        }
        tmp_entry.key = g_strndup(map, tmp - map);
        map = tmp + 1;

        /* At start of 'prepend' field */
        tmp = strchr(map, sep);
        if (!tmp) {
            fuse_log(FUSE_LOG_ERR,
                     "%s: Missing '%c' at end of prepend field of rule %zu",
                     __func__, sep, lo->xattr_map_nentries);
            exit(1);
        }
        tmp_entry.prepend = g_strndup(map, tmp - map);
        map = tmp + 1;

        add_xattrmap_entry(lo, &tmp_entry);
        /* End of rule - go around again for another rule */
    }

    if (!lo->xattr_map_nentries) {
        fuse_log(FUSE_LOG_ERR, "Empty xattr map\n");
        exit(1);
    }
}