int diskutil_unloop(const char *lodev)
{
    int i = 0;
    int ret = EUCA_OK;
    int retried = 0;
    char *output = NULL;
    boolean do_log = FALSE;

    if (lodev) {
        LOGDEBUG("detaching from loop device %s\n", lodev);

        // we retry because we have seen spurious errors from 'losetup -d' on Xen:
        //     ioctl: LOOP_CLR_FD: Device or resource bus
        for (i = 0; i < LOOP_RETRIES; i++) {
            do_log = ((i + 1) == LOOP_RETRIES); // log error on last try only
            sem_p(loop_sem);
            {
                output = pruntf(do_log, "%s %s -d %s", helpers_path[ROOTWRAP], helpers_path[LOSETUP], lodev);
            }
            sem_v(loop_sem);

            if (!output) {
                ret = EUCA_ERROR;
            } else {
                ret = EUCA_OK;
                EUCA_FREE(output);
                break;
            }

            LOGDEBUG("cannot detach loop device %s (will retry)\n", lodev);
            retried++;
            sleep(1);
        }

        if (ret == EUCA_ERROR) {
            LOGWARN("cannot detach loop device\n");
        } else if (retried) {
            LOGINFO("succeeded to detach %s after %d retries\n", lodev, retried);
        }

        return (ret);
    }

    LOGWARN("cannot detach loop device. lodev=%s\n", SP(lodev));
    return (EUCA_INVALID_ERROR);
}