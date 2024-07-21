static int lockState(const char *stateFilename, int skip_state_lock)
{
    int lockFd;

    if (!strcmp(stateFilename, "/dev/null")) {
        return 0;
    }

    lockFd = open(stateFilename, O_RDWR | O_CLOEXEC);
    if (lockFd == -1) {
        if (errno == ENOENT) {
            message(MESS_DEBUG, "Creating stub state file: %s\n",
                    stateFilename);

            /* create a stub state file with mode 0644 */
            lockFd = open(stateFilename, O_CREAT | O_EXCL | O_WRONLY,
                          S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
            if (lockFd == -1) {
                message(MESS_ERROR, "error creating stub state file %s: %s\n",
                        stateFilename, strerror(errno));
                return 1;
            }
        } else {
            message(MESS_ERROR, "error opening state file %s: %s\n",
                    stateFilename, strerror(errno));
            return 1;
        }
    }

    if (skip_state_lock) {
        message(MESS_DEBUG, "Skip locking state file %s\n",
                stateFilename);
        close(lockFd);
        return 0;
    }

    if (flock(lockFd, LOCK_EX | LOCK_NB) == -1) {
        if (errno == EWOULDBLOCK) {
            message(MESS_ERROR, "state file %s is already locked\n"
                    "logrotate does not support parallel execution on the"
                    " same set of logfiles.\n", stateFilename);
        } else {
            message(MESS_ERROR, "error acquiring lock on state file %s: %s\n",
                    stateFilename, strerror(errno));
        }
        close(lockFd);
        return 1;
    }

    /* keep lockFd open till we terminate */
    return 0;
}