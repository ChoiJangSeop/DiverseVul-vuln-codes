int bus_exec_context_set_transient_property(
                Unit *u,
                ExecContext *c,
                const char *name,
                sd_bus_message *message,
                UnitWriteFlags flags,
                sd_bus_error *error) {

        const char *suffix;
        int r;

        assert(u);
        assert(c);
        assert(name);
        assert(message);

        flags |= UNIT_PRIVATE;

        if (streq(name, "User"))
                return bus_set_transient_user(u, name, &c->user, message, flags, error);

        if (streq(name, "Group"))
                return bus_set_transient_user(u, name, &c->group, message, flags, error);

        if (streq(name, "TTYPath"))
                return bus_set_transient_path(u, name, &c->tty_path, message, flags, error);

        if (streq(name, "RootImage"))
                return bus_set_transient_path(u, name, &c->root_image, message, flags, error);

        if (streq(name, "RootDirectory"))
                return bus_set_transient_path(u, name, &c->root_directory, message, flags, error);

        if (streq(name, "SyslogIdentifier"))
                return bus_set_transient_string(u, name, &c->syslog_identifier, message, flags, error);

        if (streq(name, "LogLevelMax"))
                return bus_set_transient_log_level(u, name, &c->log_level_max, message, flags, error);

        if (streq(name, "LogRateLimitIntervalUSec"))
                return bus_set_transient_usec(u, name, &c->log_rate_limit_interval_usec, message, flags, error);

        if (streq(name, "LogRateLimitBurst"))
                return bus_set_transient_unsigned(u, name, &c->log_rate_limit_burst, message, flags, error);

        if (streq(name, "Personality"))
                return bus_set_transient_personality(u, name, &c->personality, message, flags, error);

        if (streq(name, "StandardInput"))
                return bus_set_transient_std_input(u, name, &c->std_input, message, flags, error);

        if (streq(name, "StandardOutput"))
                return bus_set_transient_std_output(u, name, &c->std_output, message, flags, error);

        if (streq(name, "StandardError"))
                return bus_set_transient_std_output(u, name, &c->std_error, message, flags, error);

        if (streq(name, "IgnoreSIGPIPE"))
                return bus_set_transient_bool(u, name, &c->ignore_sigpipe, message, flags, error);

        if (streq(name, "TTYVHangup"))
                return bus_set_transient_bool(u, name, &c->tty_vhangup, message, flags, error);

        if (streq(name, "TTYReset"))
                return bus_set_transient_bool(u, name, &c->tty_reset, message, flags, error);

        if (streq(name, "TTYVTDisallocate"))
                return bus_set_transient_bool(u, name, &c->tty_vt_disallocate, message, flags, error);

        if (streq(name, "PrivateTmp"))
                return bus_set_transient_bool(u, name, &c->private_tmp, message, flags, error);

        if (streq(name, "PrivateDevices"))
                return bus_set_transient_bool(u, name, &c->private_devices, message, flags, error);

        if (streq(name, "PrivateMounts"))
                return bus_set_transient_bool(u, name, &c->private_mounts, message, flags, error);

        if (streq(name, "PrivateNetwork"))
                return bus_set_transient_bool(u, name, &c->private_network, message, flags, error);

        if (streq(name, "PrivateUsers"))
                return bus_set_transient_bool(u, name, &c->private_users, message, flags, error);

        if (streq(name, "NoNewPrivileges"))
                return bus_set_transient_bool(u, name, &c->no_new_privileges, message, flags, error);

        if (streq(name, "SyslogLevelPrefix"))
                return bus_set_transient_bool(u, name, &c->syslog_level_prefix, message, flags, error);

        if (streq(name, "MemoryDenyWriteExecute"))
                return bus_set_transient_bool(u, name, &c->memory_deny_write_execute, message, flags, error);

        if (streq(name, "RestrictRealtime"))
                return bus_set_transient_bool(u, name, &c->restrict_realtime, message, flags, error);

        if (streq(name, "DynamicUser"))
                return bus_set_transient_bool(u, name, &c->dynamic_user, message, flags, error);

        if (streq(name, "RemoveIPC"))
                return bus_set_transient_bool(u, name, &c->remove_ipc, message, flags, error);

        if (streq(name, "ProtectKernelTunables"))
                return bus_set_transient_bool(u, name, &c->protect_kernel_tunables, message, flags, error);

        if (streq(name, "ProtectKernelModules"))
                return bus_set_transient_bool(u, name, &c->protect_kernel_modules, message, flags, error);

        if (streq(name, "ProtectControlGroups"))
                return bus_set_transient_bool(u, name, &c->protect_control_groups, message, flags, error);

        if (streq(name, "MountAPIVFS"))
                return bus_set_transient_bool(u, name, &c->mount_apivfs, message, flags, error);

        if (streq(name, "CPUSchedulingResetOnFork"))
                return bus_set_transient_bool(u, name, &c->cpu_sched_reset_on_fork, message, flags, error);

        if (streq(name, "NonBlocking"))
                return bus_set_transient_bool(u, name, &c->non_blocking, message, flags, error);

        if (streq(name, "LockPersonality"))
                return bus_set_transient_bool(u, name, &c->lock_personality, message, flags, error);

        if (streq(name, "ProtectHostname"))
                return bus_set_transient_bool(u, name, &c->protect_hostname, message, flags, error);

        if (streq(name, "UtmpIdentifier"))
                return bus_set_transient_string(u, name, &c->utmp_id, message, flags, error);

        if (streq(name, "UtmpMode"))
                return bus_set_transient_utmp_mode(u, name, &c->utmp_mode, message, flags, error);

        if (streq(name, "PAMName"))
                return bus_set_transient_string(u, name, &c->pam_name, message, flags, error);

        if (streq(name, "TimerSlackNSec"))
                return bus_set_transient_nsec(u, name, &c->timer_slack_nsec, message, flags, error);

        if (streq(name, "ProtectSystem"))
                return bus_set_transient_protect_system(u, name, &c->protect_system, message, flags, error);

        if (streq(name, "ProtectHome"))
                return bus_set_transient_protect_home(u, name, &c->protect_home, message, flags, error);

        if (streq(name, "KeyringMode"))
                return bus_set_transient_keyring_mode(u, name, &c->keyring_mode, message, flags, error);

        if (streq(name, "RuntimeDirectoryPreserve"))
                return bus_set_transient_preserve_mode(u, name, &c->runtime_directory_preserve_mode, message, flags, error);

        if (streq(name, "UMask"))
                return bus_set_transient_mode_t(u, name, &c->umask, message, flags, error);

        if (streq(name, "RuntimeDirectoryMode"))
                return bus_set_transient_mode_t(u, name, &c->directories[EXEC_DIRECTORY_RUNTIME].mode, message, flags, error);

        if (streq(name, "StateDirectoryMode"))
                return bus_set_transient_mode_t(u, name, &c->directories[EXEC_DIRECTORY_STATE].mode, message, flags, error);

        if (streq(name, "CacheDirectoryMode"))
                return bus_set_transient_mode_t(u, name, &c->directories[EXEC_DIRECTORY_CACHE].mode, message, flags, error);

        if (streq(name, "LogsDirectoryMode"))
                return bus_set_transient_mode_t(u, name, &c->directories[EXEC_DIRECTORY_LOGS].mode, message, flags, error);

        if (streq(name, "ConfigurationDirectoryMode"))
                return bus_set_transient_mode_t(u, name, &c->directories[EXEC_DIRECTORY_CONFIGURATION].mode, message, flags, error);

        if (streq(name, "SELinuxContext"))
                return bus_set_transient_string(u, name, &c->selinux_context, message, flags, error);

        if (streq(name, "SecureBits"))
                return bus_set_transient_secure_bits(u, name, &c->secure_bits, message, flags, error);

        if (streq(name, "CapabilityBoundingSet"))
                return bus_set_transient_capability(u, name, &c->capability_bounding_set, message, flags, error);

        if (streq(name, "AmbientCapabilities"))
                return bus_set_transient_capability(u, name, &c->capability_ambient_set, message, flags, error);

        if (streq(name, "RestrictNamespaces"))
                return bus_set_transient_namespace_flag(u, name, &c->restrict_namespaces, message, flags, error);

        if (streq(name, "MountFlags"))
                return bus_set_transient_mount_flags(u, name, &c->mount_flags, message, flags, error);

        if (streq(name, "NetworkNamespacePath"))
                return bus_set_transient_path(u, name, &c->network_namespace_path, message, flags, error);

        if (streq(name, "SupplementaryGroups")) {
                _cleanup_strv_free_ char **l = NULL;
                char **p;

                r = sd_bus_message_read_strv(message, &l);
                if (r < 0)
                        return r;

                STRV_FOREACH(p, l) {
                        if (!isempty(*p) && !valid_user_group_name_or_id(*p))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid supplementary group names");
                }

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        if (strv_isempty(l)) {
                                c->supplementary_groups = strv_free(c->supplementary_groups);
                                unit_write_settingf(u, flags, name, "%s=", name);
                        } else {
                                _cleanup_free_ char *joined = NULL;

                                r = strv_extend_strv(&c->supplementary_groups, l, true);
                                if (r < 0)
                                        return -ENOMEM;

                                joined = strv_join(c->supplementary_groups, " ");
                                if (!joined)
                                        return -ENOMEM;

                                unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "%s=%s", name, joined);
                        }
                }

                return 1;

        } else if (streq(name, "SyslogLevel")) {
                int32_t level;

                r = sd_bus_message_read(message, "i", &level);
                if (r < 0)
                        return r;

                if (!log_level_is_valid(level))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Log level value out of range");

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        c->syslog_priority = (c->syslog_priority & LOG_FACMASK) | level;
                        unit_write_settingf(u, flags, name, "SyslogLevel=%i", level);
                }

                return 1;

        } else if (streq(name, "SyslogFacility")) {
                int32_t facility;

                r = sd_bus_message_read(message, "i", &facility);
                if (r < 0)
                        return r;

                if (!log_facility_unshifted_is_valid(facility))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Log facility value out of range");

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        c->syslog_priority = (facility << 3) | LOG_PRI(c->syslog_priority);
                        unit_write_settingf(u, flags, name, "SyslogFacility=%i", facility);
                }

                return 1;

        } else if (streq(name, "LogExtraFields")) {
                size_t n = 0;

                r = sd_bus_message_enter_container(message, 'a', "ay");
                if (r < 0)
                        return r;

                for (;;) {
                        _cleanup_free_ void *copy = NULL;
                        struct iovec *t;
                        const char *eq;
                        const void *p;
                        size_t sz;

                        /* Note that we expect a byte array for each field, instead of a string. That's because on the
                         * lower-level journal fields can actually contain binary data and are not restricted to text,
                         * and we should not "lose precision" in our types on the way. That said, I am pretty sure
                         * actually encoding binary data as unit metadata is not a good idea. Hence we actually refuse
                         * any actual binary data, and only accept UTF-8. This allows us to eventually lift this
                         * limitation, should a good, valid usecase arise. */

                        r = sd_bus_message_read_array(message, 'y', &p, &sz);
                        if (r < 0)
                                return r;
                        if (r == 0)
                                break;

                        if (memchr(p, 0, sz))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Journal field contains zero byte");

                        eq = memchr(p, '=', sz);
                        if (!eq)
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Journal field contains no '=' character");
                        if (!journal_field_valid(p, eq - (const char*) p, false))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Journal field invalid");

                        if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                                t = reallocarray(c->log_extra_fields, c->n_log_extra_fields+1, sizeof(struct iovec));
                                if (!t)
                                        return -ENOMEM;
                                c->log_extra_fields = t;
                        }

                        copy = malloc(sz + 1);
                        if (!copy)
                                return -ENOMEM;

                        memcpy(copy, p, sz);
                        ((uint8_t*) copy)[sz] = 0;

                        if (!utf8_is_valid(copy))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Journal field is not valid UTF-8");

                        if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                                c->log_extra_fields[c->n_log_extra_fields++] = IOVEC_MAKE(copy, sz);
                                unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS|UNIT_ESCAPE_C, name, "LogExtraFields=%s", (char*) copy);

                                copy = NULL;
                        }

                        n++;
                }

                r = sd_bus_message_exit_container(message);
                if (r < 0)
                        return r;

                if (!UNIT_WRITE_FLAGS_NOOP(flags) && n == 0) {
                        exec_context_free_log_extra_fields(c);
                        unit_write_setting(u, flags, name, "LogExtraFields=");
                }

                return 1;
        }

#if HAVE_SECCOMP

        if (streq(name, "SystemCallErrorNumber"))
                return bus_set_transient_errno(u, name, &c->syscall_errno, message, flags, error);

        if (streq(name, "SystemCallFilter")) {
                int whitelist;
                _cleanup_strv_free_ char **l = NULL;

                r = sd_bus_message_enter_container(message, 'r', "bas");
                if (r < 0)
                        return r;

                r = sd_bus_message_read(message, "b", &whitelist);
                if (r < 0)
                        return r;

                r = sd_bus_message_read_strv(message, &l);
                if (r < 0)
                        return r;

                r = sd_bus_message_exit_container(message);
                if (r < 0)
                        return r;

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        _cleanup_free_ char *joined = NULL;
                        bool invert = !whitelist;
                        char **s;

                        if (strv_isempty(l)) {
                                c->syscall_whitelist = false;
                                c->syscall_filter = hashmap_free(c->syscall_filter);

                                unit_write_settingf(u, flags, name, "SystemCallFilter=");
                                return 1;
                        }

                        if (!c->syscall_filter) {
                                c->syscall_filter = hashmap_new(NULL);
                                if (!c->syscall_filter)
                                        return log_oom();

                                c->syscall_whitelist = whitelist;

                                if (c->syscall_whitelist) {
                                        r = seccomp_parse_syscall_filter("@default", -1, c->syscall_filter, SECCOMP_PARSE_WHITELIST | (invert ? SECCOMP_PARSE_INVERT : 0));
                                        if (r < 0)
                                                return r;
                                }
                        }

                        STRV_FOREACH(s, l) {
                                _cleanup_free_ char *n = NULL;
                                int e;

                                r = parse_syscall_and_errno(*s, &n, &e);
                                if (r < 0)
                                        return r;

                                r = seccomp_parse_syscall_filter(n, e, c->syscall_filter, (invert ? SECCOMP_PARSE_INVERT : 0) | (c->syscall_whitelist ? SECCOMP_PARSE_WHITELIST : 0));
                                if (r < 0)
                                        return r;
                        }

                        joined = strv_join(l, " ");
                        if (!joined)
                                return -ENOMEM;

                        unit_write_settingf(u, flags, name, "SystemCallFilter=%s%s", whitelist ? "" : "~", joined);
                }

                return 1;

        } else if (streq(name, "SystemCallArchitectures")) {
                _cleanup_strv_free_ char **l = NULL;

                r = sd_bus_message_read_strv(message, &l);
                if (r < 0)
                        return r;

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        _cleanup_free_ char *joined = NULL;

                        if (strv_isempty(l))
                                c->syscall_archs = set_free(c->syscall_archs);
                        else {
                                char **s;

                                r = set_ensure_allocated(&c->syscall_archs, NULL);
                                if (r < 0)
                                        return r;

                                STRV_FOREACH(s, l) {
                                        uint32_t a;

                                        r = seccomp_arch_from_string(*s, &a);
                                        if (r < 0)
                                                return r;

                                        r = set_put(c->syscall_archs, UINT32_TO_PTR(a + 1));
                                        if (r < 0)
                                                return r;
                                }

                        }

                        joined = strv_join(l, " ");
                        if (!joined)
                                return -ENOMEM;

                        unit_write_settingf(u, flags, name, "%s=%s", name, joined);
                }

                return 1;

        } else if (streq(name, "RestrictAddressFamilies")) {
                int whitelist;
                _cleanup_strv_free_ char **l = NULL;

                r = sd_bus_message_enter_container(message, 'r', "bas");
                if (r < 0)
                        return r;

                r = sd_bus_message_read(message, "b", &whitelist);
                if (r < 0)
                        return r;

                r = sd_bus_message_read_strv(message, &l);
                if (r < 0)
                        return r;

                r = sd_bus_message_exit_container(message);
                if (r < 0)
                        return r;

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        _cleanup_free_ char *joined = NULL;
                        bool invert = !whitelist;
                        char **s;

                        if (strv_isempty(l)) {
                                c->address_families_whitelist = false;
                                c->address_families = set_free(c->address_families);

                                unit_write_settingf(u, flags, name, "RestrictAddressFamilies=");
                                return 1;
                        }

                        if (!c->address_families) {
                                c->address_families = set_new(NULL);
                                if (!c->address_families)
                                        return log_oom();

                                c->address_families_whitelist = whitelist;
                        }

                        STRV_FOREACH(s, l) {
                                int af;

                                af = af_from_name(*s);
                                if (af < 0)
                                        return af;

                                if (!invert == c->address_families_whitelist) {
                                        r = set_put(c->address_families, INT_TO_PTR(af));
                                        if (r < 0)
                                                return r;
                                } else
                                        (void) set_remove(c->address_families, INT_TO_PTR(af));
                        }

                        joined = strv_join(l, " ");
                        if (!joined)
                                return -ENOMEM;

                        unit_write_settingf(u, flags, name, "RestrictAddressFamilies=%s%s", whitelist ? "" : "~", joined);
                }

                return 1;
        }
#endif
        if (streq(name, "CPUAffinity")) {
                const void *a;
                size_t n = 0;

                r = sd_bus_message_read_array(message, 'y', &a, &n);
                if (r < 0)
                        return r;

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        if (n == 0) {
                                c->cpuset = cpu_set_mfree(c->cpuset);
                                c->cpuset_ncpus = 0;
                                unit_write_settingf(u, flags, name, "%s=", name);
                        } else {
                                _cleanup_free_ char *str = NULL;
                                size_t allocated = 0, len = 0, i, ncpus;

                                ncpus = CPU_SIZE_TO_NUM(n);

                                for (i = 0; i < ncpus; i++) {
                                        _cleanup_free_ char *p = NULL;
                                        size_t add;

                                        if (!CPU_ISSET_S(i, n, (cpu_set_t*) a))
                                                continue;

                                        r = asprintf(&p, "%zu", i);
                                        if (r < 0)
                                                return -ENOMEM;

                                        add = strlen(p);

                                        if (!GREEDY_REALLOC(str, allocated, len + add + 2))
                                                return -ENOMEM;

                                        strcpy(mempcpy(str + len, p, add), " ");
                                        len += add + 1;
                                }

                                if (len != 0)
                                        str[len - 1] = '\0';

                                if (!c->cpuset || c->cpuset_ncpus < ncpus) {
                                        cpu_set_t *cpuset;

                                        cpuset = CPU_ALLOC(ncpus);
                                        if (!cpuset)
                                                return -ENOMEM;

                                        CPU_ZERO_S(n, cpuset);
                                        if (c->cpuset) {
                                                CPU_OR_S(CPU_ALLOC_SIZE(c->cpuset_ncpus), cpuset, c->cpuset, (cpu_set_t*) a);
                                                CPU_FREE(c->cpuset);
                                        } else
                                                CPU_OR_S(n, cpuset, cpuset, (cpu_set_t*) a);

                                        c->cpuset = cpuset;
                                        c->cpuset_ncpus = ncpus;
                                } else
                                        CPU_OR_S(n, c->cpuset, c->cpuset, (cpu_set_t*) a);

                                unit_write_settingf(u, flags, name, "%s=%s", name, str);
                        }
                }

                return 1;

        } else if (streq(name, "Nice")) {
                int32_t q;

                r = sd_bus_message_read(message, "i", &q);
                if (r < 0)
                        return r;

                if (!nice_is_valid(q))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid Nice value: %i", q);

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        c->nice = q;
                        c->nice_set = true;

                        unit_write_settingf(u, flags, name, "Nice=%i", q);
                }

                return 1;

        } else if (streq(name, "CPUSchedulingPolicy")) {
                int32_t q;

                r = sd_bus_message_read(message, "i", &q);
                if (r < 0)
                        return r;

                if (!sched_policy_is_valid(q))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid CPU scheduling policy: %i", q);

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        _cleanup_free_ char *s = NULL;

                        r = sched_policy_to_string_alloc(q, &s);
                        if (r < 0)
                                return r;

                        c->cpu_sched_policy = q;
                        c->cpu_sched_priority = CLAMP(c->cpu_sched_priority, sched_get_priority_min(q), sched_get_priority_max(q));
                        c->cpu_sched_set = true;

                        unit_write_settingf(u, flags, name, "CPUSchedulingPolicy=%s", s);
                }

                return 1;

        } else if (streq(name, "CPUSchedulingPriority")) {
                int32_t p, min, max;

                r = sd_bus_message_read(message, "i", &p);
                if (r < 0)
                        return r;

                min = sched_get_priority_min(c->cpu_sched_policy);
                max = sched_get_priority_max(c->cpu_sched_policy);
                if (p < min || p > max)
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid CPU scheduling priority: %i", p);

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        c->cpu_sched_priority = p;
                        c->cpu_sched_set = true;

                        unit_write_settingf(u, flags, name, "CPUSchedulingPriority=%i", p);
                }

                return 1;

        } else if (streq(name, "IOSchedulingClass")) {
                int32_t q;

                r = sd_bus_message_read(message, "i", &q);
                if (r < 0)
                        return r;

                if (!ioprio_class_is_valid(q))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid IO scheduling class: %i", q);

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        _cleanup_free_ char *s = NULL;

                        r = ioprio_class_to_string_alloc(q, &s);
                        if (r < 0)
                                return r;

                        c->ioprio = IOPRIO_PRIO_VALUE(q, IOPRIO_PRIO_DATA(c->ioprio));
                        c->ioprio_set = true;

                        unit_write_settingf(u, flags, name, "IOSchedulingClass=%s", s);
                }

                return 1;

        } else if (streq(name, "IOSchedulingPriority")) {
                int32_t p;

                r = sd_bus_message_read(message, "i", &p);
                if (r < 0)
                        return r;

                if (!ioprio_priority_is_valid(p))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid IO scheduling priority: %i", p);

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        c->ioprio = IOPRIO_PRIO_VALUE(IOPRIO_PRIO_CLASS(c->ioprio), p);
                        c->ioprio_set = true;

                        unit_write_settingf(u, flags, name, "IOSchedulingPriority=%i", p);
                }

                return 1;

        } else if (streq(name, "WorkingDirectory")) {
                const char *s;
                bool missing_ok;

                r = sd_bus_message_read(message, "s", &s);
                if (r < 0)
                        return r;

                if (s[0] == '-') {
                        missing_ok = true;
                        s++;
                } else
                        missing_ok = false;

                if (!isempty(s) && !streq(s, "~") && !path_is_absolute(s))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "WorkingDirectory= expects an absolute path or '~'");

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        if (streq(s, "~")) {
                                c->working_directory = mfree(c->working_directory);
                                c->working_directory_home = true;
                        } else {
                                r = free_and_strdup(&c->working_directory, empty_to_null(s));
                                if (r < 0)
                                        return r;

                                c->working_directory_home = false;
                        }

                        c->working_directory_missing_ok = missing_ok;
                        unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "WorkingDirectory=%s%s", missing_ok ? "-" : "", s);
                }

                return 1;

        } else if (STR_IN_SET(name,
                              "StandardInputFileDescriptorName", "StandardOutputFileDescriptorName", "StandardErrorFileDescriptorName")) {
                const char *s;

                r = sd_bus_message_read(message, "s", &s);
                if (r < 0)
                        return r;

                if (!isempty(s) && !fdname_is_valid(s))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid file descriptor name");

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {

                        if (streq(name, "StandardInputFileDescriptorName")) {
                                r = free_and_strdup(c->stdio_fdname + STDIN_FILENO, empty_to_null(s));
                                if (r < 0)
                                        return r;

                                c->std_input = EXEC_INPUT_NAMED_FD;
                                unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "StandardInput=fd:%s", exec_context_fdname(c, STDIN_FILENO));

                        } else if (streq(name, "StandardOutputFileDescriptorName")) {
                                r = free_and_strdup(c->stdio_fdname + STDOUT_FILENO, empty_to_null(s));
                                if (r < 0)
                                        return r;

                                c->std_output = EXEC_OUTPUT_NAMED_FD;
                                unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "StandardOutput=fd:%s", exec_context_fdname(c, STDOUT_FILENO));

                        } else {
                                assert(streq(name, "StandardErrorFileDescriptorName"));

                                r = free_and_strdup(&c->stdio_fdname[STDERR_FILENO], empty_to_null(s));
                                if (r < 0)
                                        return r;

                                c->std_error = EXEC_OUTPUT_NAMED_FD;
                                unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "StandardError=fd:%s", exec_context_fdname(c, STDERR_FILENO));
                        }
                }

                return 1;

        } else if (STR_IN_SET(name,
                              "StandardInputFile",
                              "StandardOutputFile", "StandardOutputFileToAppend",
                              "StandardErrorFile", "StandardErrorFileToAppend")) {
                const char *s;

                r = sd_bus_message_read(message, "s", &s);
                if (r < 0)
                        return r;

                if (!isempty(s)) {
                        if (!path_is_absolute(s))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Path %s is not absolute", s);
                        if (!path_is_normalized(s))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Path %s is not normalized", s);
                }

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {

                        if (streq(name, "StandardInputFile")) {
                                r = free_and_strdup(&c->stdio_file[STDIN_FILENO], empty_to_null(s));
                                if (r < 0)
                                        return r;

                                c->std_input = EXEC_INPUT_FILE;
                                unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "StandardInput=file:%s", s);

                        } else if (STR_IN_SET(name, "StandardOutputFile", "StandardOutputFileToAppend")) {
                                r = free_and_strdup(&c->stdio_file[STDOUT_FILENO], empty_to_null(s));
                                if (r < 0)
                                        return r;

                                if (streq(name, "StandardOutputFile")) {
                                        c->std_output = EXEC_OUTPUT_FILE;
                                        unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "StandardOutput=file:%s", s);
                                } else {
                                        assert(streq(name, "StandardOutputFileToAppend"));
                                        c->std_output = EXEC_OUTPUT_FILE_APPEND;
                                        unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "StandardOutput=append:%s", s);
                                }
                        } else {
                                assert(STR_IN_SET(name, "StandardErrorFile", "StandardErrorFileToAppend"));

                                r = free_and_strdup(&c->stdio_file[STDERR_FILENO], empty_to_null(s));
                                if (r < 0)
                                        return r;

                                if (streq(name, "StandardErrorFile")) {
                                        c->std_error = EXEC_OUTPUT_FILE;
                                        unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "StandardError=file:%s", s);
                                } else {
                                        assert(streq(name, "StandardErrorFileToAppend"));
                                        c->std_error = EXEC_OUTPUT_FILE_APPEND;
                                        unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "StandardError=append:%s", s);
                                }
                        }
                }

                return 1;

        } else if (streq(name, "StandardInputData")) {
                const void *p;
                size_t sz;

                r = sd_bus_message_read_array(message, 'y', &p, &sz);
                if (r < 0)
                        return r;

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        _cleanup_free_ char *encoded = NULL;

                        if (sz == 0) {
                                c->stdin_data = mfree(c->stdin_data);
                                c->stdin_data_size = 0;

                                unit_write_settingf(u, flags, name, "StandardInputData=");
                        } else {
                                void *q;
                                ssize_t n;

                                if (c->stdin_data_size + sz < c->stdin_data_size || /* check for overflow */
                                    c->stdin_data_size + sz > EXEC_STDIN_DATA_MAX)
                                        return -E2BIG;

                                n = base64mem(p, sz, &encoded);
                                if (n < 0)
                                        return (int) n;

                                q = realloc(c->stdin_data, c->stdin_data_size + sz);
                                if (!q)
                                        return -ENOMEM;

                                memcpy((uint8_t*) q + c->stdin_data_size, p, sz);

                                c->stdin_data = q;
                                c->stdin_data_size += sz;

                                unit_write_settingf(u, flags, name, "StandardInputData=%s", encoded);
                        }
                }

                return 1;

        } else if (streq(name, "Environment")) {

                _cleanup_strv_free_ char **l = NULL;

                r = sd_bus_message_read_strv(message, &l);
                if (r < 0)
                        return r;

                if (!strv_env_is_valid(l))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid environment block.");

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        if (strv_isempty(l)) {
                                c->environment = strv_free(c->environment);
                                unit_write_setting(u, flags, name, "Environment=");
                        } else {
                                _cleanup_free_ char *joined = NULL;
                                char **e;

                                joined = unit_concat_strv(l, UNIT_ESCAPE_SPECIFIERS|UNIT_ESCAPE_C);
                                if (!joined)
                                        return -ENOMEM;

                                e = strv_env_merge(2, c->environment, l);
                                if (!e)
                                        return -ENOMEM;

                                strv_free_and_replace(c->environment, e);
                                unit_write_settingf(u, flags, name, "Environment=%s", joined);
                        }
                }

                return 1;

        } else if (streq(name, "UnsetEnvironment")) {

                _cleanup_strv_free_ char **l = NULL;

                r = sd_bus_message_read_strv(message, &l);
                if (r < 0)
                        return r;

                if (!strv_env_name_or_assignment_is_valid(l))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid UnsetEnvironment= list.");

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        if (strv_isempty(l)) {
                                c->unset_environment = strv_free(c->unset_environment);
                                unit_write_setting(u, flags, name, "UnsetEnvironment=");
                        } else {
                                _cleanup_free_ char *joined = NULL;
                                char **e;

                                joined = unit_concat_strv(l, UNIT_ESCAPE_SPECIFIERS|UNIT_ESCAPE_C);
                                if (!joined)
                                        return -ENOMEM;

                                e = strv_env_merge(2, c->unset_environment, l);
                                if (!e)
                                        return -ENOMEM;

                                strv_free_and_replace(c->unset_environment, e);
                                unit_write_settingf(u, flags, name, "UnsetEnvironment=%s", joined);
                        }
                }

                return 1;

        } else if (streq(name, "OOMScoreAdjust")) {
                int oa;

                r = sd_bus_message_read(message, "i", &oa);
                if (r < 0)
                        return r;

                if (!oom_score_adjust_is_valid(oa))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "OOM score adjust value out of range");

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        c->oom_score_adjust = oa;
                        c->oom_score_adjust_set = true;
                        unit_write_settingf(u, flags, name, "OOMScoreAdjust=%i", oa);
                }

                return 1;

        } else if (streq(name, "EnvironmentFiles")) {

                _cleanup_free_ char *joined = NULL;
                _cleanup_fclose_ FILE *f = NULL;
                _cleanup_strv_free_ char **l = NULL;
                size_t size = 0;
                char **i;

                r = sd_bus_message_enter_container(message, 'a', "(sb)");
                if (r < 0)
                        return r;

                f = open_memstream(&joined, &size);
                if (!f)
                        return -ENOMEM;

                (void) __fsetlocking(f, FSETLOCKING_BYCALLER);

                fputs("EnvironmentFile=\n", f);

                STRV_FOREACH(i, c->environment_files) {
                        _cleanup_free_ char *q = NULL;

                        q = specifier_escape(*i);
                        if (!q)
                                return -ENOMEM;

                        fprintf(f, "EnvironmentFile=%s\n", q);
                }

                while ((r = sd_bus_message_enter_container(message, 'r', "sb")) > 0) {
                        const char *path;
                        int b;

                        r = sd_bus_message_read(message, "sb", &path, &b);
                        if (r < 0)
                                return r;

                        r = sd_bus_message_exit_container(message);
                        if (r < 0)
                                return r;

                        if (!path_is_absolute(path))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Path %s is not absolute.", path);

                        if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                                _cleanup_free_ char *q = NULL;
                                char *buf;

                                buf = strjoin(b ? "-" : "", path);
                                if (!buf)
                                        return -ENOMEM;

                                q = specifier_escape(buf);
                                if (!q) {
                                        free(buf);
                                        return -ENOMEM;
                                }

                                fprintf(f, "EnvironmentFile=%s\n", q);

                                r = strv_consume(&l, buf);
                                if (r < 0)
                                        return r;
                        }
                }
                if (r < 0)
                        return r;

                r = sd_bus_message_exit_container(message);
                if (r < 0)
                        return r;

                r = fflush_and_check(f);
                if (r < 0)
                        return r;

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        if (strv_isempty(l)) {
                                c->environment_files = strv_free(c->environment_files);
                                unit_write_setting(u, flags, name, "EnvironmentFile=");
                        } else {
                                r = strv_extend_strv(&c->environment_files, l, true);
                                if (r < 0)
                                        return r;

                                unit_write_setting(u, flags, name, joined);
                        }
                }

                return 1;

        } else if (streq(name, "PassEnvironment")) {

                _cleanup_strv_free_ char **l = NULL;

                r = sd_bus_message_read_strv(message, &l);
                if (r < 0)
                        return r;

                if (!strv_env_name_is_valid(l))
                        return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid PassEnvironment= block.");

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        if (strv_isempty(l)) {
                                c->pass_environment = strv_free(c->pass_environment);
                                unit_write_setting(u, flags, name, "PassEnvironment=");
                        } else {
                                _cleanup_free_ char *joined = NULL;

                                r = strv_extend_strv(&c->pass_environment, l, true);
                                if (r < 0)
                                        return r;

                                /* We write just the new settings out to file, with unresolved specifiers. */
                                joined = unit_concat_strv(l, UNIT_ESCAPE_SPECIFIERS);
                                if (!joined)
                                        return -ENOMEM;

                                unit_write_settingf(u, flags, name, "PassEnvironment=%s", joined);
                        }
                }

                return 1;

        } else if (STR_IN_SET(name, "ReadWriteDirectories", "ReadOnlyDirectories", "InaccessibleDirectories",
                              "ReadWritePaths", "ReadOnlyPaths", "InaccessiblePaths")) {
                _cleanup_strv_free_ char **l = NULL;
                char ***dirs;
                char **p;

                r = sd_bus_message_read_strv(message, &l);
                if (r < 0)
                        return r;

                STRV_FOREACH(p, l) {
                        char *i = *p;
                        size_t offset;

                        offset = i[0] == '-';
                        offset += i[offset] == '+';
                        if (!path_is_absolute(i + offset))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Invalid %s", name);

                        path_simplify(i + offset, false);
                }

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        if (STR_IN_SET(name, "ReadWriteDirectories", "ReadWritePaths"))
                                dirs = &c->read_write_paths;
                        else if (STR_IN_SET(name, "ReadOnlyDirectories", "ReadOnlyPaths"))
                                dirs = &c->read_only_paths;
                        else /* "InaccessiblePaths" */
                                dirs = &c->inaccessible_paths;

                        if (strv_isempty(l)) {
                                *dirs = strv_free(*dirs);
                                unit_write_settingf(u, flags, name, "%s=", name);
                        } else {
                                _cleanup_free_ char *joined = NULL;

                                joined = unit_concat_strv(l, UNIT_ESCAPE_SPECIFIERS);
                                if (!joined)
                                        return -ENOMEM;

                                r = strv_extend_strv(dirs, l, true);
                                if (r < 0)
                                        return -ENOMEM;

                                unit_write_settingf(u, flags, name, "%s=%s", name, joined);
                        }
                }

                return 1;

        } else if (STR_IN_SET(name, "RuntimeDirectory", "StateDirectory", "CacheDirectory", "LogsDirectory", "ConfigurationDirectory")) {
                _cleanup_strv_free_ char **l = NULL;
                char **p;

                r = sd_bus_message_read_strv(message, &l);
                if (r < 0)
                        return r;

                STRV_FOREACH(p, l) {
                        if (!path_is_normalized(*p))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "%s= path is not normalized: %s", name, *p);

                        if (path_is_absolute(*p))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "%s= path is absolute: %s", name, *p);

                        if (path_startswith(*p, "private"))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "%s= path can't be 'private': %s", name, *p);
                }

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        ExecDirectoryType i;
                        ExecDirectory *d;

                        assert_se((i = exec_directory_type_from_string(name)) >= 0);
                        d = c->directories + i;

                        if (strv_isempty(l)) {
                                d->paths = strv_free(d->paths);
                                unit_write_settingf(u, flags, name, "%s=", name);
                        } else {
                                _cleanup_free_ char *joined = NULL;

                                r = strv_extend_strv(&d->paths, l, true);
                                if (r < 0)
                                        return r;

                                joined = unit_concat_strv(l, UNIT_ESCAPE_SPECIFIERS);
                                if (!joined)
                                        return -ENOMEM;

                                unit_write_settingf(u, flags, name, "%s=%s", name, joined);
                        }
                }

                return 1;

        } else if (STR_IN_SET(name, "AppArmorProfile", "SmackProcessLabel")) {
                int ignore;
                const char *s;

                r = sd_bus_message_read(message, "(bs)", &ignore, &s);
                if (r < 0)
                        return r;

                if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                        char **p;
                        bool *b;

                        if (streq(name, "AppArmorProfile")) {
                                p = &c->apparmor_profile;
                                b = &c->apparmor_profile_ignore;
                        } else { /* "SmackProcessLabel" */
                                p = &c->smack_process_label;
                                b = &c->smack_process_label_ignore;
                        }

                        if (isempty(s)) {
                                *p = mfree(*p);
                                *b = false;
                        } else {
                                if (free_and_strdup(p, s) < 0)
                                        return -ENOMEM;
                                *b = ignore;
                        }

                        unit_write_settingf(u, flags|UNIT_ESCAPE_SPECIFIERS, name, "%s=%s%s", name, ignore ? "-" : "", strempty(s));
                }

                return 1;

        } else if (STR_IN_SET(name, "BindPaths", "BindReadOnlyPaths")) {
                const char *source, *destination;
                int ignore_enoent;
                uint64_t mount_flags;
                bool empty = true;

                r = sd_bus_message_enter_container(message, 'a', "(ssbt)");
                if (r < 0)
                        return r;

                while ((r = sd_bus_message_read(message, "(ssbt)", &source, &destination, &ignore_enoent, &mount_flags)) > 0) {

                        if (!path_is_absolute(source))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Source path %s is not absolute.", source);
                        if (!path_is_absolute(destination))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Destination path %s is not absolute.", destination);
                        if (!IN_SET(mount_flags, 0, MS_REC))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Unknown mount flags.");

                        if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                                r = bind_mount_add(&c->bind_mounts, &c->n_bind_mounts,
                                                   &(BindMount) {
                                                           .source = strdup(source),
                                                           .destination = strdup(destination),
                                                           .read_only = !!strstr(name, "ReadOnly"),
                                                           .recursive = !!(mount_flags & MS_REC),
                                                           .ignore_enoent = ignore_enoent,
                                                   });
                                if (r < 0)
                                        return r;

                                unit_write_settingf(
                                                u, flags|UNIT_ESCAPE_SPECIFIERS, name,
                                                "%s=%s%s:%s:%s",
                                                name,
                                                ignore_enoent ? "-" : "",
                                                source,
                                                destination,
                                                (mount_flags & MS_REC) ? "rbind" : "norbind");
                        }

                        empty = false;
                }
                if (r < 0)
                        return r;

                r = sd_bus_message_exit_container(message);
                if (r < 0)
                        return r;

                if (empty) {
                        bind_mount_free_many(c->bind_mounts, c->n_bind_mounts);
                        c->bind_mounts = NULL;
                        c->n_bind_mounts = 0;

                        unit_write_settingf(u, flags, name, "%s=", name);
                }

                return 1;

        } else if (streq(name, "TemporaryFileSystem")) {
                const char *path, *options;
                bool empty = true;

                r = sd_bus_message_enter_container(message, 'a', "(ss)");
                if (r < 0)
                        return r;

                while ((r = sd_bus_message_read(message, "(ss)", &path, &options)) > 0) {

                        if (!path_is_absolute(path))
                                return sd_bus_error_setf(error, SD_BUS_ERROR_INVALID_ARGS, "Mount point %s is not absolute.", path);

                        if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                                r = temporary_filesystem_add(&c->temporary_filesystems, &c->n_temporary_filesystems, path, options);
                                if (r < 0)
                                        return r;

                                unit_write_settingf(
                                                u, flags|UNIT_ESCAPE_SPECIFIERS, name,
                                                "%s=%s:%s",
                                                name,
                                                path,
                                                options);
                        }

                        empty = false;
                }
                if (r < 0)
                        return r;

                r = sd_bus_message_exit_container(message);
                if (r < 0)
                        return r;

                if (empty) {
                        temporary_filesystem_free_many(c->temporary_filesystems, c->n_temporary_filesystems);
                        c->temporary_filesystems = NULL;
                        c->n_temporary_filesystems = 0;

                        unit_write_settingf(u, flags, name, "%s=", name);
                }

                return 1;

        } else if ((suffix = startswith(name, "Limit"))) {
                const char *soft = NULL;
                int ri;

                ri = rlimit_from_string(suffix);
                if (ri < 0) {
                        soft = endswith(suffix, "Soft");
                        if (soft) {
                                const char *n;

                                n = strndupa(suffix, soft - suffix);
                                ri = rlimit_from_string(n);
                                if (ri >= 0)
                                        name = strjoina("Limit", n);
                        }
                }

                if (ri >= 0) {
                        uint64_t rl;
                        rlim_t x;

                        r = sd_bus_message_read(message, "t", &rl);
                        if (r < 0)
                                return r;

                        if (rl == (uint64_t) -1)
                                x = RLIM_INFINITY;
                        else {
                                x = (rlim_t) rl;

                                if ((uint64_t) x != rl)
                                        return -ERANGE;
                        }

                        if (!UNIT_WRITE_FLAGS_NOOP(flags)) {
                                _cleanup_free_ char *f = NULL;
                                struct rlimit nl;

                                if (c->rlimit[ri]) {
                                        nl = *c->rlimit[ri];

                                        if (soft)
                                                nl.rlim_cur = x;
                                        else
                                                nl.rlim_max = x;
                                } else
                                        /* When the resource limit is not initialized yet, then assign the value to both fields */
                                        nl = (struct rlimit) {
                                                .rlim_cur = x,
                                                .rlim_max = x,
                                        };

                                r = rlimit_format(&nl, &f);
                                if (r < 0)
                                        return r;

                                if (c->rlimit[ri])
                                        *c->rlimit[ri] = nl;
                                else {
                                        c->rlimit[ri] = newdup(struct rlimit, &nl, 1);
                                        if (!c->rlimit[ri])
                                                return -ENOMEM;
                                }

                                unit_write_settingf(u, flags, name, "%s=%s", name, f);
                        }

                        return 1;
                }

        }

        return 0;
}