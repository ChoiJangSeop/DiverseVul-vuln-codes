netsnmp_mibindex_lookup( const char *dirname )
{
    int i;
    static char tmpbuf[300];

    for (i=0; i<_mibindex; i++) {
        if ( _mibindexes[i] &&
             strcmp( _mibindexes[i], dirname ) == 0) {
             snprintf(tmpbuf, sizeof(tmpbuf), "%s/mib_indexes/%d",
                      get_persistent_directory(), i);
             tmpbuf[sizeof(tmpbuf)-1] = 0;
             DEBUGMSGTL(("mibindex", "lookup: %s (%d) %s\n", dirname, i, tmpbuf ));
             return tmpbuf;
        }
    }
    DEBUGMSGTL(("mibindex", "lookup: (none)\n"));
    return NULL;
}