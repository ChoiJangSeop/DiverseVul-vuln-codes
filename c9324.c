add_mibdir(const char *dirname)
{
    FILE           *ip;
    const char     *oldFile = File;
    char          **filenames;
    int             count = 0;
    int             filename_count, i;
#if !(defined(WIN32) || defined(cygwin))
    char           *token;
    char space;
    char newline;
    struct stat     dir_stat, idx_stat;
    char            tmpstr[300];
    char            tmpstr1[300];
#endif

    DEBUGMSGTL(("parse-mibs", "Scanning directory %s\n", dirname));
#if !(defined(WIN32) || defined(cygwin))
    token = netsnmp_mibindex_lookup( dirname );
    if (token && stat(token, &idx_stat) == 0 && stat(dirname, &dir_stat) == 0) {
        if (dir_stat.st_mtime < idx_stat.st_mtime) {
            DEBUGMSGTL(("parse-mibs", "The index is good\n"));
            if ((ip = fopen(token, "r")) != NULL) {
                fgets(tmpstr, sizeof(tmpstr), ip); /* Skip dir line */
                while (fscanf(ip, "%127s%c%299[^\n]%c", token, &space, tmpstr,
		    &newline) == 4) {

		    /*
		     * If an overflow of the token or tmpstr buffers has been
		     * found log a message and break out of the while loop,
		     * thus the rest of the file tokens will be ignored.
		     */
		    if (space != ' ' || newline != '\n') {
			snmp_log(LOG_ERR,
			    "add_mibdir: strings scanned in from %s/%s " \
			    "are too large.  count = %d\n ", dirname,
			    ".index", count);
			    break;
		    }
		   
		    snprintf(tmpstr1, sizeof(tmpstr1), "%s/%s", dirname, tmpstr);
                    tmpstr1[ sizeof(tmpstr1)-1 ] = 0;
                    new_module(token, tmpstr1);
                    count++;
                }
                fclose(ip);
                return count;
            } else
                DEBUGMSGTL(("parse-mibs", "Can't read index\n"));
        } else
            DEBUGMSGTL(("parse-mibs", "Index outdated\n"));
    } else
        DEBUGMSGTL(("parse-mibs", "No index\n"));
#endif

    filename_count = scan_directory(&filenames, dirname);

    if (filename_count >= 0) {
        ip = netsnmp_mibindex_new(dirname);
        for (i = 0; i < filename_count; i++) {
            if (add_mibfile(filenames[i], strrchr(filenames[i], '/'), ip) == 0)
                count++;
	    free(filenames[i]);
        }
        File = oldFile;
        if (ip)
            fclose(ip);
        free(filenames);
        return (count);
    }
    else
        DEBUGMSGTL(("parse-mibs","cannot open MIB directory %s\n", dirname));

    return (-1);
}