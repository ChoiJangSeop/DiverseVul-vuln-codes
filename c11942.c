int diskutil_loop(const char *path, const long long offset, char *lodev, int lodev_size)
{
    int i = 0;
    int ret = EUCA_OK;
    char *ptr = NULL;
    char *output = NULL;
    boolean done = FALSE;
    boolean found = FALSE;
    boolean do_log = FALSE;

    if (path && lodev) {
        // we retry because we cannot atomically obtain a free loopback device on all distros (some
        // versions of 'losetup' allow a file argument with '-f' options, but some do not)
        for (i = 0, done = FALSE, found = FALSE; i < LOOP_RETRIES; i++) {
            sem_p(loop_sem);
            {
                output = pruntf(TRUE, "%s %s -f", helpers_path[ROOTWRAP], helpers_path[LOSETUP]);
            }
            sem_v(loop_sem);

            if (output == NULL) {
                // there was a problem
                break;
            }

            if (strstr(output, "/dev/loop")) {
                strncpy(lodev, output, lodev_size);
                if ((ptr = strrchr(lodev, '\n')) != NULL) {
                    *ptr = '\0';
                    found = TRUE;
                }
            }

            EUCA_FREE(output);

            if (found) {
                do_log = ((i + 1) == LOOP_RETRIES); // log error on last try only
                LOGDEBUG("attaching file %s\n", path);
                LOGDEBUG("            to %s at offset %lld\n", lodev, offset);
                sem_p(loop_sem);
                {
                    output = pruntf(do_log, "%s %s -o %lld %s %s", helpers_path[ROOTWRAP], helpers_path[LOSETUP], offset, lodev, path);
                }
                sem_v(loop_sem);

                if (output == NULL) {
                    LOGDEBUG("cannot attach to loop device %s (will retry)\n", lodev);
                } else {
                    EUCA_FREE(output);
                    done = TRUE;
                    break;
                }
            }

            sleep(1);
            found = FALSE;
        }

        if (!done) {
            LOGERROR("cannot find free loop device or attach to one\n");
            ret = EUCA_ERROR;
        }

        return (ret);
    }

    LOGWARN("cannot attach to loop device. path=%s, lodev=%s\n", SP(path), SP(lodev));
    return (EUCA_INVALID_ERROR);
}