
static void dispatch_message_real(
                Server *s,
                struct iovec *iovec, size_t n, size_t m,
                const ClientContext *c,
                const struct timeval *tv,
                int priority,
                pid_t object_pid) {

        char source_time[sizeof("_SOURCE_REALTIME_TIMESTAMP=") + DECIMAL_STR_MAX(usec_t)];
        uid_t journal_uid;
        ClientContext *o;

        assert(s);
        assert(iovec);
        assert(n > 0);
        assert(n +
               N_IOVEC_META_FIELDS +
               (pid_is_valid(object_pid) ? N_IOVEC_OBJECT_FIELDS : 0) +
               client_context_extra_fields_n_iovec(c) <= m);

        if (c) {
                IOVEC_ADD_NUMERIC_FIELD(iovec, n, c->pid, pid_t, pid_is_valid, PID_FMT, "_PID");
                IOVEC_ADD_NUMERIC_FIELD(iovec, n, c->uid, uid_t, uid_is_valid, UID_FMT, "_UID");
                IOVEC_ADD_NUMERIC_FIELD(iovec, n, c->gid, gid_t, gid_is_valid, GID_FMT, "_GID");

                IOVEC_ADD_STRING_FIELD(iovec, n, c->comm, "_COMM");
                IOVEC_ADD_STRING_FIELD(iovec, n, c->exe, "_EXE");
                IOVEC_ADD_STRING_FIELD(iovec, n, c->cmdline, "_CMDLINE");
                IOVEC_ADD_STRING_FIELD(iovec, n, c->capeff, "_CAP_EFFECTIVE");

                IOVEC_ADD_SIZED_FIELD(iovec, n, c->label, c->label_size, "_SELINUX_CONTEXT");

                IOVEC_ADD_NUMERIC_FIELD(iovec, n, c->auditid, uint32_t, audit_session_is_valid, "%" PRIu32, "_AUDIT_SESSION");
                IOVEC_ADD_NUMERIC_FIELD(iovec, n, c->loginuid, uid_t, uid_is_valid, UID_FMT, "_AUDIT_LOGINUID");

                IOVEC_ADD_STRING_FIELD(iovec, n, c->cgroup, "_SYSTEMD_CGROUP");
                IOVEC_ADD_STRING_FIELD(iovec, n, c->session, "_SYSTEMD_SESSION");
                IOVEC_ADD_NUMERIC_FIELD(iovec, n, c->owner_uid, uid_t, uid_is_valid, UID_FMT, "_SYSTEMD_OWNER_UID");
                IOVEC_ADD_STRING_FIELD(iovec, n, c->unit, "_SYSTEMD_UNIT");
                IOVEC_ADD_STRING_FIELD(iovec, n, c->user_unit, "_SYSTEMD_USER_UNIT");
                IOVEC_ADD_STRING_FIELD(iovec, n, c->slice, "_SYSTEMD_SLICE");
                IOVEC_ADD_STRING_FIELD(iovec, n, c->user_slice, "_SYSTEMD_USER_SLICE");

                IOVEC_ADD_ID128_FIELD(iovec, n, c->invocation_id, "_SYSTEMD_INVOCATION_ID");

                if (c->extra_fields_n_iovec > 0) {
                        memcpy(iovec + n, c->extra_fields_iovec, c->extra_fields_n_iovec * sizeof(struct iovec));
                        n += c->extra_fields_n_iovec;
                }
        }

        assert(n <= m);

        if (pid_is_valid(object_pid) && client_context_get(s, object_pid, NULL, NULL, 0, NULL, &o) >= 0) {

                IOVEC_ADD_NUMERIC_FIELD(iovec, n, o->pid, pid_t, pid_is_valid, PID_FMT, "OBJECT_PID");
                IOVEC_ADD_NUMERIC_FIELD(iovec, n, o->uid, uid_t, uid_is_valid, UID_FMT, "OBJECT_UID");
                IOVEC_ADD_NUMERIC_FIELD(iovec, n, o->gid, gid_t, gid_is_valid, GID_FMT, "OBJECT_GID");

                IOVEC_ADD_STRING_FIELD(iovec, n, o->comm, "OBJECT_COMM");
                IOVEC_ADD_STRING_FIELD(iovec, n, o->exe, "OBJECT_EXE");
                IOVEC_ADD_STRING_FIELD(iovec, n, o->cmdline, "OBJECT_CMDLINE");
                IOVEC_ADD_STRING_FIELD(iovec, n, o->capeff, "OBJECT_CAP_EFFECTIVE");

                IOVEC_ADD_SIZED_FIELD(iovec, n, o->label, o->label_size, "OBJECT_SELINUX_CONTEXT");

                IOVEC_ADD_NUMERIC_FIELD(iovec, n, o->auditid, uint32_t, audit_session_is_valid, "%" PRIu32, "OBJECT_AUDIT_SESSION");
                IOVEC_ADD_NUMERIC_FIELD(iovec, n, o->loginuid, uid_t, uid_is_valid, UID_FMT, "OBJECT_AUDIT_LOGINUID");

                IOVEC_ADD_STRING_FIELD(iovec, n, o->cgroup, "OBJECT_SYSTEMD_CGROUP");
                IOVEC_ADD_STRING_FIELD(iovec, n, o->session, "OBJECT_SYSTEMD_SESSION");
                IOVEC_ADD_NUMERIC_FIELD(iovec, n, o->owner_uid, uid_t, uid_is_valid, UID_FMT, "OBJECT_SYSTEMD_OWNER_UID");
                IOVEC_ADD_STRING_FIELD(iovec, n, o->unit, "OBJECT_SYSTEMD_UNIT");
                IOVEC_ADD_STRING_FIELD(iovec, n, o->user_unit, "OBJECT_SYSTEMD_USER_UNIT");
                IOVEC_ADD_STRING_FIELD(iovec, n, o->slice, "OBJECT_SYSTEMD_SLICE");
                IOVEC_ADD_STRING_FIELD(iovec, n, o->user_slice, "OBJECT_SYSTEMD_USER_SLICE");

                IOVEC_ADD_ID128_FIELD(iovec, n, o->invocation_id, "OBJECT_SYSTEMD_INVOCATION_ID=");
        }

        assert(n <= m);

        if (tv) {
                sprintf(source_time, "_SOURCE_REALTIME_TIMESTAMP=" USEC_FMT, timeval_load(tv));
                iovec[n++] = IOVEC_MAKE_STRING(source_time);
        }

        /* Note that strictly speaking storing the boot id here is
         * redundant since the entry includes this in-line
         * anyway. However, we need this indexed, too. */
        if (!isempty(s->boot_id_field))
                iovec[n++] = IOVEC_MAKE_STRING(s->boot_id_field);

        if (!isempty(s->machine_id_field))
                iovec[n++] = IOVEC_MAKE_STRING(s->machine_id_field);

        if (!isempty(s->hostname_field))
                iovec[n++] = IOVEC_MAKE_STRING(s->hostname_field);

        assert(n <= m);

        if (s->split_mode == SPLIT_UID && c && uid_is_valid(c->uid))
                /* Split up strictly by (non-root) UID */
                journal_uid = c->uid;
        else if (s->split_mode == SPLIT_LOGIN && c && c->uid > 0 && uid_is_valid(c->owner_uid))
                /* Split up by login UIDs.  We do this only if the
                 * realuid is not root, in order not to accidentally
                 * leak privileged information to the user that is
                 * logged by a privileged process that is part of an
                 * unprivileged session. */
                journal_uid = c->owner_uid;
        else
                journal_uid = 0;

        write_to_journal(s, journal_uid, iovec, n, priority);