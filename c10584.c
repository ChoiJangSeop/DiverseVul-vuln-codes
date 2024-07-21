static errno_t sssctl_manage_service(enum sssctl_svc_action action)
{
#ifdef HAVE_SYSTEMD
    switch (action) {
    case SSSCTL_SVC_START:
        return sssctl_systemd_start();
    case SSSCTL_SVC_STOP:
        return sssctl_systemd_stop();
    case SSSCTL_SVC_RESTART:
        return sssctl_systemd_restart();
    }
#elif defined(HAVE_SERVICE)
    switch (action) {
    case SSSCTL_SVC_START:
        return sssctl_run_command(SERVICE_PATH" sssd start");
    case SSSCTL_SVC_STOP:
        return sssctl_run_command(SERVICE_PATH" sssd stop");
    case SSSCTL_SVC_RESTART:
        return sssctl_run_command(SERVICE_PATH" sssd restart");
    }
#endif

    return ENOSYS;
}