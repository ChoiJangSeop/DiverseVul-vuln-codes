int pidfile_write(pid_t pid)
{
    FILE *f;

    if (g_pidfile) {
        f = fopen(g_pidfile, "w+");
    } else if (pidfilefd >= 0) {
        f = fdopen(pidfilefd, "w");
        if (f) {
            g_pidfile = fd_to_filename(pidfilefd);
            if (!g_pidfile)
                goto error;
        }
    } else {
        return 0;
    }

    if (!f) {
        logprintf(STDERR_FILENO, "Could not open pidfile %s : %s\n",
                  g_pidfile, strerror(errno));
        goto error;
    }

    if (fprintf(f, "%d", pid) < 0) {
        logprintf(STDERR_FILENO, "Could not write to pidfile : %s\n",
                  strerror(errno));
        goto error;
    }

    fclose(f);

    return 0;

error:
    if (f)
        fclose(f);
    return -1;
}