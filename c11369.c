static int writeState(const char *stateFilename)
{
    struct logState *p;
    FILE *f;
    char *chptr;
    unsigned int i = 0;
    int error = 0;
    int bytes = 0;
    int fdcurr;
    int fdsave;
    struct stat sb;
    char *tmpFilename = NULL;
    struct tm now;
    time_t now_time, last_time;
    char *prevCtx;

    if (!strcmp(stateFilename, "/dev/null"))
        /* explicitly asked not to write the state file */
        return 0;

    localtime_r(&nowSecs, &now);

    tmpFilename = malloc(strlen(stateFilename) + 5 );
    if (tmpFilename == NULL) {
        message_OOM();
        return 1;
    }
    strcpy(tmpFilename, stateFilename);
    strcat(tmpFilename, ".tmp");
    /* Remove possible tmp state file from previous run */
    error = unlink(tmpFilename);
    if (error == -1 && errno != ENOENT) {
        message(MESS_ERROR, "error removing old temporary state file %s: %s\n",
                tmpFilename, strerror(errno));
        free(tmpFilename);
        return 1;
    }
    error = 0;

    fdcurr = open(stateFilename, O_RDONLY);
    if (fdcurr == -1) {
        /* the statefile should exist, lockState() already created an empty
         * state file in case it did not exist initially */
        message(MESS_ERROR, "error opening state file %s: %s\n",
                stateFilename, strerror(errno));
        free(tmpFilename);
        return 1;
    }

    /* get attributes, to assign them to the new state file */

    if (setSecCtx(fdcurr, stateFilename, &prevCtx) != 0) {
        /* error msg already printed */
        free(tmpFilename);
        close(fdcurr);
        return 1;
    }

#ifdef WITH_ACL
    if ((prev_acl = acl_get_fd(fdcurr)) == NULL) {
        if (is_acl_well_supported(errno)) {
            message(MESS_ERROR, "getting file ACL %s: %s\n",
                    stateFilename, strerror(errno));
            restoreSecCtx(&prevCtx);
            free(tmpFilename);
            close(fdcurr);
            return 1;
        }
    }
#endif

    if (fstat(fdcurr, &sb) == -1) {
        message(MESS_ERROR, "error stating %s: %s\n", stateFilename, strerror(errno));
        restoreSecCtx(&prevCtx);
        free(tmpFilename);
#ifdef WITH_ACL
        if (prev_acl) {
            acl_free(prev_acl);
            prev_acl = NULL;
        }
#endif
        return 1;
    }

    close(fdcurr);

    /* drop world-readable flag to prevent others from locking */
    sb.st_mode &= ~(mode_t)S_IROTH;

    fdsave = createOutputFile(tmpFilename, O_RDWR, &sb, prev_acl, 0);
#ifdef WITH_ACL
    if (prev_acl) {
        acl_free(prev_acl);
        prev_acl = NULL;
    }
#endif
    restoreSecCtx(&prevCtx);

    if (fdsave < 0) {
        free(tmpFilename);
        return 1;
    }

    f = fdopen(fdsave, "w");
    if (!f) {
        message(MESS_ERROR, "error creating temp state file %s: %s\n",
                tmpFilename, strerror(errno));
        free(tmpFilename);
        return 1;
    }

    bytes =  fprintf(f, "logrotate state -- version 2\n");
    if (bytes < 0)
        error = bytes;

    /*
     * Time in seconds it takes earth to go around sun.  The value is
     * astronomical measurement (solar year) rather than something derived from
     * a convention (calendar year).
     */
#define SECONDS_IN_YEAR 31556926

    for (i = 0; i < hashSize && error == 0; i++) {
        for (p = states[i]->head.lh_first; p != NULL && error == 0;
                p = p->list.le_next) {

            /* Skip states which are not used for more than a year. */
            now_time = mktime(&now);
            last_time = mktime(&p->lastRotated);
            if (!p->isUsed && difftime(now_time, last_time) > SECONDS_IN_YEAR) {
                message(MESS_DEBUG, "Removing %s from state file, "
                        "because it does not exist and has not been rotated for one year\n",
                        p->fn);
                continue;
            }

            error = fputc('"', f) == EOF;
            for (chptr = p->fn; *chptr && error == 0; chptr++) {
                switch (*chptr) {
                    case '"':
                    case '\\':
                        error = fputc('\\', f) == EOF;
                        break;
                    case '\n':
                        error = fputc('\\', f) == EOF;
                        if (error == 0) {
                            error = fputc('n', f) == EOF;
                        }
                        continue;
                    default:
                        break;
                }
                if (error == 0 && fputc(*chptr, f) == EOF) {
                    error = 1;
                }
            }

            if (error == 0 && fputc('"', f) == EOF)
                error = 1;

            if (error == 0) {
                bytes = fprintf(f, " %d-%d-%d-%d:%d:%d\n",
                                p->lastRotated.tm_year + 1900,
                                p->lastRotated.tm_mon + 1,
                                p->lastRotated.tm_mday,
                                p->lastRotated.tm_hour,
                                p->lastRotated.tm_min,
                                p->lastRotated.tm_sec);
                if (bytes < 0)
                    error = bytes;
            }
        }
    }

    if (error == 0)
        error = fflush(f);

    if (error == 0)
        error = fsync(fdsave);

    if (error == 0)
        error = fclose(f);
    else
        fclose(f);

    if (error == 0) {
        if (rename(tmpFilename, stateFilename)) {
            message(MESS_ERROR, "error renaming temp state file %s to %s: %s\n",
                    tmpFilename, stateFilename, strerror(errno));
            unlink(tmpFilename);
            error = 1;
        }
    }
    else {
        if (errno)
            message(MESS_ERROR, "error creating temp state file %s: %s\n",
                    tmpFilename, strerror(errno));
        else
            message(MESS_ERROR, "error creating temp state file %s%s\n",
                    tmpFilename, error == ENOMEM ?
                    ": Insufficient storage space is available." : "" );
        unlink(tmpFilename);
    }
    free(tmpFilename);
    return error;
}