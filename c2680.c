static time_t parse_time_file(const char *filename)
{
    /* Open input file, and parse it. */
    int fd = open(filename, O_RDONLY | O_NOFOLLOW);
    if (fd < 0)
    {
        VERB2 pwarn_msg("Can't open '%s'", filename);
        return -1;
    }

    /* ~ maximal number of digits for positive time stamp string */
    char time_buf[sizeof(time_t) * 3 + 1];
    ssize_t rdsz = read(fd, time_buf, sizeof(time_buf));

    /* Just reading, so no need to check the returned value. */
    close(fd);

    if (rdsz == -1)
    {
        VERB2 pwarn_msg("Can't read from '%s'", filename);
        return -1;
    }
    /* approximate maximal number of digits in timestamp is sizeof(time_t)*3 */
    /* buffer has this size + 1 byte for trailing '\0' */
    /* if whole size of buffer was read then file is bigger */
    /* than string representing maximal time stamp */
    if (rdsz == sizeof(time_buf))
    {
        VERB2 warn_msg("File '%s' is too long to be valid unix "
                       "time stamp (max size %u)", filename, (int)sizeof(time_buf));
        return -1;
    }

    /* Our tools don't put trailing newline into time file,
     * but we allow such format too:
     */
    if (rdsz > 0 && time_buf[rdsz - 1] == '\n')
        rdsz--;
    time_buf[rdsz] = '\0';

    /* Note that on some architectures (x32) time_t is "long long" */

    errno = 0;    /* To distinguish success/failure after call */
    char *endptr;
    long long val = strtoll(time_buf, &endptr, /* base */ 10);
    const long long MAX_TIME_T = (1ULL << (sizeof(time_t)*8 - 1)) - 1;

    /* Check for various possible errors */
    if (errno
     || (*endptr != '\0')
     || val >= MAX_TIME_T
     || !isdigit_str(time_buf) /* this filters out "-num", "   num", "" */
    ) {
        VERB2 pwarn_msg("File '%s' doesn't contain valid unix "
                        "time stamp ('%s')", filename, time_buf);
        return -1;
    }

    /* If we got here, strtoll() successfully parsed a number */
    return val;
}