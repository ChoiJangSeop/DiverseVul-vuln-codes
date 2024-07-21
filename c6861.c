static int gather_pid_metadata(
                char* context[_CONTEXT_MAX],
                char **comm_fallback,
                struct iovec *iovec, size_t *n_iovec) {

        /* We need 27 empty slots in iovec!
         *
         * Note that if we fail on oom later on, we do not roll-back changes to the iovec structure. (It remains valid,
         * with the first n_iovec fields initialized.) */

        uid_t owner_uid;
        pid_t pid;
        char *t;
        const char *p;
        int r, signo;

        r = parse_pid(context[CONTEXT_PID], &pid);
        if (r < 0)
                return log_error_errno(r, "Failed to parse PID \"%s\": %m", context[CONTEXT_PID]);

        r = get_process_comm(pid, &context[CONTEXT_COMM]);
        if (r < 0) {
                log_warning_errno(r, "Failed to get COMM, falling back to the command line: %m");
                context[CONTEXT_COMM] = strv_join(comm_fallback, " ");
                if (!context[CONTEXT_COMM])
                        return log_oom();
        }

        r = get_process_exe(pid, &context[CONTEXT_EXE]);
        if (r < 0)
                log_warning_errno(r, "Failed to get EXE, ignoring: %m");

        if (cg_pid_get_unit(pid, &context[CONTEXT_UNIT]) >= 0) {
                if (!is_journald_crash((const char**) context)) {
                        /* OK, now we know it's not the journal, hence we can make use of it now. */
                        log_set_target(LOG_TARGET_JOURNAL_OR_KMSG);
                        log_open();
                }

                /* If this is PID 1 disable coredump collection, we'll unlikely be able to process it later on. */
                if (is_pid1_crash((const char**) context)) {
                        log_notice("Due to PID 1 having crashed coredump collection will now be turned off.");
                        disable_coredumps();
                }

                set_iovec_field(iovec, n_iovec, "COREDUMP_UNIT=", context[CONTEXT_UNIT]);
        }

        if (cg_pid_get_user_unit(pid, &t) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_USER_UNIT=", t);

        /* The next few are mandatory */
        if (!set_iovec_field(iovec, n_iovec, "COREDUMP_PID=", context[CONTEXT_PID]))
                return log_oom();

        if (!set_iovec_field(iovec, n_iovec, "COREDUMP_UID=", context[CONTEXT_UID]))
                return log_oom();

        if (!set_iovec_field(iovec, n_iovec, "COREDUMP_GID=", context[CONTEXT_GID]))
                return log_oom();

        if (!set_iovec_field(iovec, n_iovec, "COREDUMP_SIGNAL=", context[CONTEXT_SIGNAL]))
                return log_oom();

        if (!set_iovec_field(iovec, n_iovec, "COREDUMP_RLIMIT=", context[CONTEXT_RLIMIT]))
                return log_oom();

        if (!set_iovec_field(iovec, n_iovec, "COREDUMP_HOSTNAME=", context[CONTEXT_HOSTNAME]))
                return log_oom();

        if (!set_iovec_field(iovec, n_iovec, "COREDUMP_COMM=", context[CONTEXT_COMM]))
                return log_oom();

        if (context[CONTEXT_EXE] &&
            !set_iovec_field(iovec, n_iovec, "COREDUMP_EXE=", context[CONTEXT_EXE]))
                return log_oom();

        if (sd_pid_get_session(pid, &t) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_SESSION=", t);

        if (sd_pid_get_owner_uid(pid, &owner_uid) >= 0) {
                r = asprintf(&t, "COREDUMP_OWNER_UID=" UID_FMT, owner_uid);
                if (r > 0)
                        iovec[(*n_iovec)++] = IOVEC_MAKE_STRING(t);
        }

        if (sd_pid_get_slice(pid, &t) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_SLICE=", t);

        if (get_process_cmdline(pid, 0, false, &t) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_CMDLINE=", t);

        if (cg_pid_get_path_shifted(pid, NULL, &t) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_CGROUP=", t);

        if (compose_open_fds(pid, &t) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_OPEN_FDS=", t);

        p = procfs_file_alloca(pid, "status");
        if (read_full_file(p, &t, NULL) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_PROC_STATUS=", t);

        p = procfs_file_alloca(pid, "maps");
        if (read_full_file(p, &t, NULL) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_PROC_MAPS=", t);

        p = procfs_file_alloca(pid, "limits");
        if (read_full_file(p, &t, NULL) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_PROC_LIMITS=", t);

        p = procfs_file_alloca(pid, "cgroup");
        if (read_full_file(p, &t, NULL) >=0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_PROC_CGROUP=", t);

        p = procfs_file_alloca(pid, "mountinfo");
        if (read_full_file(p, &t, NULL) >=0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_PROC_MOUNTINFO=", t);

        if (get_process_cwd(pid, &t) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_CWD=", t);

        if (get_process_root(pid, &t) >= 0) {
                bool proc_self_root_is_slash;

                proc_self_root_is_slash = strcmp(t, "/") == 0;

                set_iovec_field_free(iovec, n_iovec, "COREDUMP_ROOT=", t);

                /* If the process' root is "/", then there is a chance it has
                 * mounted own root and hence being containerized. */
                if (proc_self_root_is_slash && get_process_container_parent_cmdline(pid, &t) > 0)
                        set_iovec_field_free(iovec, n_iovec, "COREDUMP_CONTAINER_CMDLINE=", t);
        }

        if (get_process_environ(pid, &t) >= 0)
                set_iovec_field_free(iovec, n_iovec, "COREDUMP_ENVIRON=", t);

        t = strjoin("COREDUMP_TIMESTAMP=", context[CONTEXT_TIMESTAMP], "000000");
        if (t)
                iovec[(*n_iovec)++] = IOVEC_MAKE_STRING(t);

        if (safe_atoi(context[CONTEXT_SIGNAL], &signo) >= 0 && SIGNAL_VALID(signo))
                set_iovec_field(iovec, n_iovec, "COREDUMP_SIGNAL_NAME=SIG", signal_to_string(signo));

        return 0; /* we successfully acquired all metadata */
}