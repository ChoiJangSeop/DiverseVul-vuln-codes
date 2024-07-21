void usage(const char *progname)
{
    const char* progname_real; /* contains the real name of the program */
                               /*  (without path) */

    progname_real = strrchr(progname, '/');

    if (progname_real == NULL) /* no path in progname: use progname */
    {
        progname_real = progname;
    }
    else
    {
        progname_real++;
    }

    fprintf(stderr, "\nusage: %s {-e|-d} [ { -p <password> | -k <keyfile> } ] { [-o <output filename>] <file> | <file> [<file> ...] }\n\n",
            progname_real);
}