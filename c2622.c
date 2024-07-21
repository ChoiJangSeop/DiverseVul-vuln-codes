static void save_bt_to_dump_dir(const char *bt, const char *exe, const char *reason)
{
    time_t t = time(NULL);
    const char *iso_date = iso_date_string(&t);
    /* dump should be readable by all if we're run with -x */
    uid_t my_euid = (uid_t)-1L;
    mode_t mode = DEFAULT_DUMP_DIR_MODE | S_IROTH;
    /* and readable only for the owner otherwise */
    if (!(g_opts & OPT_x))
    {
        mode = DEFAULT_DUMP_DIR_MODE;
        my_euid = geteuid();
    }

    pid_t my_pid = getpid();

    char base[sizeof("xorg-YYYY-MM-DD-hh:mm:ss-%lu-%lu") + 2 * sizeof(long)*3];
    sprintf(base, "xorg-%s-%lu-%u", iso_date, (long)my_pid, g_bt_count);
    char *path = concat_path_file(debug_dumps_dir, base);

    struct dump_dir *dd = dd_create(path, /*uid:*/ my_euid, mode);
    if (dd)
    {
        dd_create_basic_files(dd, /*uid:*/ my_euid, NULL);
        dd_save_text(dd, FILENAME_ABRT_VERSION, VERSION);
        dd_save_text(dd, FILENAME_ANALYZER, "xorg");
        dd_save_text(dd, FILENAME_TYPE, "xorg");
        dd_save_text(dd, FILENAME_REASON, reason);
        dd_save_text(dd, FILENAME_BACKTRACE, bt);
        /*
         * Reporters usually need component name to file a bug.
	 * It is usually derived from executable.
         * We _guess_ X server's executable name as a last resort.
         * Better ideas?
         */
        if (!exe)
        {
            exe = "/usr/bin/X";
            if (access("/usr/bin/Xorg", X_OK) == 0)
                exe = "/usr/bin/Xorg";
        }
        dd_save_text(dd, FILENAME_EXECUTABLE, exe);
        dd_close(dd);
        notify_new_path(path);
    }

    free(path);
}