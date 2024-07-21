static int readState(const char *stateFilename)
{
    FILE *f;
    char buf[STATEFILE_BUFFER_SIZE];
    char *filename;
    const char **argv;
    int argc;
    int year, month, day, hour, minute, second;
    int line = 0;
    int fd;
    struct logState *st;
    time_t lr_time;
    struct stat f_stat;
    int rc = 0;

    message(MESS_DEBUG, "Reading state from file: %s\n", stateFilename);

    fd = open(stateFilename, O_RDONLY);
    if (fd == -1) {
        /* treat non-openable file as an empty file for allocateHash() */
        f_stat.st_size = 0;

        /* no error if state just not exists */
        if (errno != ENOENT) {
            message(MESS_ERROR, "error opening state file %s: %s\n",
                    stateFilename, strerror(errno));

            /* do not return until the hash table is allocated */
            rc = 1;
        }
    } else {
        if (fstat(fd, &f_stat) == -1) {
            /* treat non-statable file as an empty file for allocateHash() */
            f_stat.st_size = 0;

            message(MESS_ERROR, "error stat()ing state file %s: %s\n",
                    stateFilename, strerror(errno));

            /* do not return until the hash table is allocated */
            rc = 1;
        }
    }

    /* Try to estimate how many state entries we have in the state file.
     * We expect single entry to have around 80 characters (Of course this is
     * just an estimation). During the testing I've found out that 200 entries
     * per single hash entry gives good mem/performance ratio. */
    if (allocateHash((size_t)f_stat.st_size / 80 / 200))
        rc = 1;

    if (rc || (f_stat.st_size == 0)) {
        /* error already occurred, or we have no state file to read from */
        if (fd != -1)
            close(fd);
        return rc;
    }

    f = fdopen(fd, "r");
    if (!f) {
        message(MESS_ERROR, "error opening state file %s: %s\n",
                stateFilename, strerror(errno));
        close(fd);
        return 1;
    }

    if (!fgets(buf, sizeof(buf) - 1, f)) {
        message(MESS_ERROR, "error reading top line of %s\n",
                stateFilename);
        fclose(f);
        return 1;
    }

    if (strcmp(buf, "logrotate state -- version 1\n") != 0 &&
            strcmp(buf, "logrotate state -- version 2\n") != 0) {
        fclose(f);
        message(MESS_ERROR, "bad top line in state file %s\n",
                stateFilename);
        return 1;
    }

    line++;

    while (fgets(buf, sizeof(buf) - 1, f)) {
        const size_t i = strlen(buf);
        argv = NULL;
        line++;
        if (i == 0) {
            message(MESS_ERROR, "line %d not parsable in state file %s\n",
                    line, stateFilename);
            fclose(f);
            return 1;
        }
        if (buf[i - 1] != '\n') {
            message(MESS_ERROR, "line %d too long in state file %s\n",
                    line, stateFilename);
            fclose(f);
            return 1;
        }

        buf[i - 1] = '\0';

        if (i == 1)
            continue;

        year = month = day = hour = minute = second = 0;
        if (poptParseArgvString(buf, &argc, &argv) || (argc != 2) ||
                (sscanf(argv[1], "%d-%d-%d-%d:%d:%d", &year, &month, &day, &hour, &minute, &second) < 3)) {
            message(MESS_ERROR, "bad line %d in state file %s\n",
                    line, stateFilename);
            free(argv);
            fclose(f);
            return 1;
        }

        /* Hack to hide earlier bug */
        if ((year != 1900) && (year < 1970 || year > 2100)) {
            message(MESS_ERROR,
                    "bad year %d for file %s in state file %s\n", year,
                    argv[0], stateFilename);
            free(argv);
            fclose(f);
            return 1;
        }

        if (month < 1 || month > 12) {
            message(MESS_ERROR,
                    "bad month %d for file %s in state file %s\n", month,
                    argv[0], stateFilename);
            free(argv);
            fclose(f);
            return 1;
        }

        /* 0 to hide earlier bug */
        if (day < 0 || day > 31) {
            message(MESS_ERROR,
                    "bad day %d for file %s in state file %s\n", day,
                    argv[0], stateFilename);
            free(argv);
            fclose(f);
            return 1;
        }

        if (hour < 0 || hour > 23) {
            message(MESS_ERROR,
                    "bad hour %d for file %s in state file %s\n", hour,
                    argv[0], stateFilename);
            free(argv);
            fclose(f);
            return 1;
        }

        if (minute < 0 || minute > 59) {
            message(MESS_ERROR,
                    "bad minute %d for file %s in state file %s\n", minute,
                    argv[0], stateFilename);
            free(argv);
            fclose(f);
            return 1;
        }

        if (second < 0 || second > 59) {
            message(MESS_ERROR,
                    "bad second %d for file %s in state file %s\n", second,
                    argv[0], stateFilename);
            free(argv);
            fclose(f);
            return 1;
        }

        year -= 1900;
        month -= 1;

        filename = strdup(argv[0]);
        if (filename == NULL) {
            message_OOM();
            free(argv);
            fclose(f);
            return 1;
        }
        unescape(filename);

        if ((st = findState(filename)) == NULL) {
            free(argv);
            free(filename);
            fclose(f);
            return 1;
        }

        memset(&st->lastRotated, 0, sizeof(st->lastRotated));
        st->lastRotated.tm_year = year;
        st->lastRotated.tm_mon = month;
        st->lastRotated.tm_mday = day;
        st->lastRotated.tm_hour = hour;
        st->lastRotated.tm_min = minute;
        st->lastRotated.tm_sec = second;
        st->lastRotated.tm_isdst = -1;

        /* fill in the rest of the st->lastRotated fields */
        lr_time = mktime(&st->lastRotated);
        localtime_r(&lr_time, &st->lastRotated);

        free(argv);
        free(filename);
    }

    fclose(f);
    return 0;
}