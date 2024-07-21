static bool not_valid_time_file(const char *filename)
{
    /* Open input file, and parse it. */
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        VERB2 perror_msg("Can't open '%s'", filename);
        return true;
    }

    /* ~ maximal number of digits for positive time stamp string*/
    char time_buf[sizeof(time_t) * 3 + 1];
    ssize_t rdsz = read(fd, time_buf, sizeof(time_buf));

    /* Just reading, so no need to check the returned value. */
    close(fd);

    if (rdsz == -1)
    {
        VERB2 perror_msg("Can't read from '%s'", filename);
        return true;
    }

    /* approximate maximal number of digits in timestamp is sizeof(time_t)*3 */
    /* buffer has this size + 1 byte for trailing '\0' */
    /* if whole size of buffer was read then file is bigger */
    /* than string representing maximal time stamp */
    if (rdsz == sizeof(time_buf))
    {
        VERB2 log("File '%s' is too long to be valid unix "
                  "time stamp (max size %zdB)", filename, sizeof(time_buf));
        return true;
    }

    /* Our tools don't put trailing newline into time file,
     * but we allow such format too:
     */
    if (rdsz > 0 && time_buf[rdsz - 1] == '\n')
        rdsz--;
    time_buf[rdsz] = '\0';

    /* the value should fit to timestamp number range because of file size */
    /* condition above, hence check if the value string consists only from digits */
    if (!isdigit_str(time_buf))
    {
        VERB2 log("File '%s' doesn't contain valid unix "
                  "time stamp ('%s')", filename, time_buf);
        return true;
    }

    return false;
}