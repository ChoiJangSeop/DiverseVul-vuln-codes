netsnmp_mibindex_load( void )
{
    DIR *dir;
    struct dirent *file;
    FILE *fp;
    char tmpbuf[ 300];
    char tmpbuf2[300];
    int  i;
    char *cp;

    /*
     * Open the MIB index directory, or create it (empty)
     */
    snprintf( tmpbuf, sizeof(tmpbuf), "%s/mib_indexes",
              get_persistent_directory());
    tmpbuf[sizeof(tmpbuf)-1] = 0;
    dir = opendir( tmpbuf );
    if ( dir == NULL ) {
        DEBUGMSGTL(("mibindex", "load: (new)\n"));
        mkdirhier( tmpbuf, NETSNMP_AGENT_DIRECTORY_MODE, 0);
        return;
    }

    /*
     * Create a list of which directory each file refers to
     */
    while ((file = readdir( dir ))) {
        if ( !isdigit((unsigned char)(file->d_name[0])))
            continue;
        i = atoi( file->d_name );

        snprintf( tmpbuf, sizeof(tmpbuf), "%s/mib_indexes/%d",
              get_persistent_directory(), i );
        tmpbuf[sizeof(tmpbuf)-1] = 0;
        fp = fopen( tmpbuf, "r" );
        if (!fp)
            continue;
        cp = fgets( tmpbuf2, sizeof(tmpbuf2), fp );
        fclose( fp );
        if ( !cp ) {
            DEBUGMSGTL(("mibindex", "Empty MIB index (%d)\n", i));
            continue;
        }
        if ( strncmp( tmpbuf2, "DIR ", 4 ) != 0 ) {
            DEBUGMSGTL(("mibindex", "Malformed MIB index (%d)\n", i));
            continue;
        }
        tmpbuf2[strlen(tmpbuf2)-1] = 0;
        DEBUGMSGTL(("mibindex", "load: (%d) %s\n", i, tmpbuf2));
        (void)_mibindex_add( tmpbuf2+4, i );  /* Skip 'DIR ' */
    }
    closedir( dir );
}