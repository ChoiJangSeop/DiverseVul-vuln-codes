netsnmp_mibindex_new( const char *dirname )
{
    FILE *fp;
    char  tmpbuf[300];
    char *cp;
    int   i;

    cp = netsnmp_mibindex_lookup( dirname );
    if (!cp) {
        i  = _mibindex_add( dirname, -1 );
        snprintf( tmpbuf, sizeof(tmpbuf), "%s/mib_indexes/%d",
                  get_persistent_directory(), i );
        tmpbuf[sizeof(tmpbuf)-1] = 0;
        cp = tmpbuf;
    }
    DEBUGMSGTL(("mibindex", "new: %s (%s)\n", dirname, cp ));
    fp = fopen( cp, "w" );
    if (fp)
        fprintf( fp, "DIR %s\n", dirname );
    return fp;
}