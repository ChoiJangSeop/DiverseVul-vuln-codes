static void usage(void)
{
    fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
    fprintf(stderr, "A tool to reconstruct mailboxes.\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "-C <config-file>   use <config-file> instead of config from imapd.conf");
    fprintf(stderr, "-p <partition>     use this indicated partition for search\n");
    fprintf(stderr, "-d                 check mailboxes database and insert missing entries\n");
    fprintf(stderr, "-x                 do not import metadata, create new\n");
    fprintf(stderr, "-r                 recursively reconstruct\n");
    fprintf(stderr, "-f                 examine filesystem underneath the mailbox\n");
    fprintf(stderr, "-s                 don't stat underlying files\n");
    fprintf(stderr, "-q                 run quietly\n");
    fprintf(stderr, "-n                 do not make changes\n");
    fprintf(stderr, "-G                 force re-parsing (checks GUID correctness)\n");
    fprintf(stderr, "-R                 perform UID upgrade operation on GUID mismatched files\n");
    fprintf(stderr, "-U                 use this if there are corrupt message files in spool\n");
    fprintf(stderr, "                   WARNING: this option deletes corrupted message files permanently\n");
    fprintf(stderr, "-o                 ignore odd files in mailbox disk directories\n");
    fprintf(stderr, "-O                 delete odd files (unlike -o)\n");
    fprintf(stderr, "-M                 prefer mailboxes.db over cyrus.header\n");
    fprintf(stderr, "-V <version>       Change the cyrus.index minor version to the version specified\n");
    fprintf(stderr, "-u                 give usernames instead of mailbox prefixes\n");

    fprintf(stderr, "\n");

    exit(EX_USAGE);
}