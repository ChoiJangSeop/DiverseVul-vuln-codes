static int bus_append_execute_property(sd_bus_message *m, const char *field, const char *eq) {
        const char *suffix;
        int r;

        if (STR_IN_SET(field,
                       "User", "Group",
                       "UtmpIdentifier", "UtmpMode", "PAMName", "TTYPath",
                       "WorkingDirectory", "RootDirectory", "SyslogIdentifier",
                       "ProtectSystem", "ProtectHome", "SELinuxContext", "RootImage",
                       "RuntimeDirectoryPreserve", "Personality", "KeyringMode", "NetworkNamespacePath"))

                return bus_append_string(m, field, eq);

        if (STR_IN_SET(field,
                       "IgnoreSIGPIPE", "TTYVHangup", "TTYReset", "TTYVTDisallocate",
                       "PrivateTmp", "PrivateDevices", "PrivateNetwork", "PrivateUsers",
                       "PrivateMounts", "NoNewPrivileges", "SyslogLevelPrefix",
                       "MemoryDenyWriteExecute", "RestrictRealtime", "DynamicUser", "RemoveIPC",
                       "ProtectKernelTunables", "ProtectKernelModules", "ProtectControlGroups",
                       "MountAPIVFS", "CPUSchedulingResetOnFork", "LockPersonality", "ProtectHostname"))

                return bus_append_parse_boolean(m, field, eq);

        if (STR_IN_SET(field,
                       "ReadWriteDirectories", "ReadOnlyDirectories", "InaccessibleDirectories",
                       "ReadWritePaths", "ReadOnlyPaths", "InaccessiblePaths",
                       "RuntimeDirectory", "StateDirectory", "CacheDirectory", "LogsDirectory", "ConfigurationDirectory",
                       "SupplementaryGroups", "SystemCallArchitectures"))

                return bus_append_strv(m, field, eq, EXTRACT_QUOTES);

        if (STR_IN_SET(field, "SyslogLevel", "LogLevelMax"))

                return bus_append_log_level_from_string(m, field, eq);

        if (streq(field, "SyslogFacility"))

                return bus_append_log_facility_unshifted_from_string(m, field, eq);

        if (streq(field, "SecureBits"))

                return bus_append_secure_bits_from_string(m, field, eq);

        if (streq(field, "CPUSchedulingPolicy"))

                return bus_append_sched_policy_from_string(m, field, eq);

        if (STR_IN_SET(field, "CPUSchedulingPriority", "OOMScoreAdjust"))

                return bus_append_safe_atoi(m, field, eq);

        if (streq(field, "Nice"))

                return bus_append_parse_nice(m, field, eq);

        if (streq(field, "SystemCallErrorNumber"))

                return bus_append_parse_errno(m, field, eq);

        if (streq(field, "IOSchedulingClass"))

                return bus_append_ioprio_class_from_string(m, field, eq);

        if (streq(field, "IOSchedulingPriority"))

                return bus_append_ioprio_parse_priority(m, field, eq);

        if (STR_IN_SET(field,
                       "RuntimeDirectoryMode", "StateDirectoryMode", "CacheDirectoryMode",
                       "LogsDirectoryMode", "ConfigurationDirectoryMode", "UMask"))

                return bus_append_parse_mode(m, field, eq);

        if (streq(field, "TimerSlackNSec"))

                return bus_append_parse_nsec(m, field, eq);

        if (streq(field, "LogRateLimitIntervalSec"))

                return bus_append_parse_sec_rename(m, field, eq);

        if (streq(field, "LogRateLimitBurst"))

                return bus_append_safe_atou(m, field, eq);

        if (streq(field, "MountFlags"))

                return bus_append_mount_propagation_flags_from_string(m, field, eq);

        if (STR_IN_SET(field, "Environment", "UnsetEnvironment", "PassEnvironment"))

                return bus_append_strv(m, field, eq, EXTRACT_QUOTES|EXTRACT_CUNESCAPE);

        if (streq(field, "EnvironmentFile")) {

                if (isempty(eq))
                        r = sd_bus_message_append(m, "(sv)", "EnvironmentFiles", "a(sb)", 0);
                else
                        r = sd_bus_message_append(m, "(sv)", "EnvironmentFiles", "a(sb)", 1,
                                                  eq[0] == '-' ? eq + 1 : eq,
                                                  eq[0] == '-');
                if (r < 0)
                        return bus_log_create_error(r);

                return 1;
        }

        if (streq(field, "LogExtraFields")) {

                r = sd_bus_message_open_container(m, 'r', "sv");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_append_basic(m, 's', "LogExtraFields");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_open_container(m, 'v', "aay");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_open_container(m, 'a', "ay");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_append_array(m, 'y', eq, strlen(eq));
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                return 1;
        }

        if (STR_IN_SET(field, "StandardInput", "StandardOutput", "StandardError")) {
                const char *n, *appended;

                if ((n = startswith(eq, "fd:"))) {
                        appended = strjoina(field, "FileDescriptorName");
                        r = sd_bus_message_append(m, "(sv)", appended, "s", n);
                } else if ((n = startswith(eq, "file:"))) {
                        appended = strjoina(field, "File");
                        r = sd_bus_message_append(m, "(sv)", appended, "s", n);
                } else if ((n = startswith(eq, "append:"))) {
                        appended = strjoina(field, "FileToAppend");
                        r = sd_bus_message_append(m, "(sv)", appended, "s", n);
                } else
                        r = sd_bus_message_append(m, "(sv)", field, "s", eq);
                if (r < 0)
                        return bus_log_create_error(r);

                return 1;
        }

        if (streq(field, "StandardInputText")) {
                _cleanup_free_ char *unescaped = NULL;

                r = cunescape(eq, 0, &unescaped);
                if (r < 0)
                        return log_error_errno(r, "Failed to unescape text '%s': %m", eq);

                if (!strextend(&unescaped, "\n", NULL))
                        return log_oom();

                /* Note that we don't expand specifiers here, but that should be OK, as this is a programmatic
                 * interface anyway */

                return bus_append_byte_array(m, field, unescaped, strlen(unescaped));
        }

        if (streq(field, "StandardInputData")) {
                _cleanup_free_ void *decoded = NULL;
                size_t sz;

                r = unbase64mem(eq, (size_t) -1, &decoded, &sz);
                if (r < 0)
                        return log_error_errno(r, "Failed to decode base64 data '%s': %m", eq);

                return bus_append_byte_array(m, field, decoded, sz);
        }

        if ((suffix = startswith(field, "Limit"))) {
                int rl;

                rl = rlimit_from_string(suffix);
                if (rl >= 0) {
                        const char *sn;
                        struct rlimit l;

                        r = rlimit_parse(rl, eq, &l);
                        if (r < 0)
                                return log_error_errno(r, "Failed to parse resource limit: %s", eq);

                        r = sd_bus_message_append(m, "(sv)", field, "t", l.rlim_max);
                        if (r < 0)
                                return bus_log_create_error(r);

                        sn = strjoina(field, "Soft");
                        r = sd_bus_message_append(m, "(sv)", sn, "t", l.rlim_cur);
                        if (r < 0)
                                return bus_log_create_error(r);

                        return 1;
                }
        }

        if (STR_IN_SET(field, "AppArmorProfile", "SmackProcessLabel")) {
                int ignore = 0;
                const char *s = eq;

                if (eq[0] == '-') {
                        ignore = 1;
                        s = eq + 1;
                }

                r = sd_bus_message_append(m, "(sv)", field, "(bs)", ignore, s);
                if (r < 0)
                        return bus_log_create_error(r);

                return 1;
        }

        if (STR_IN_SET(field, "CapabilityBoundingSet", "AmbientCapabilities")) {
                uint64_t sum = 0;
                bool invert = false;
                const char *p = eq;

                if (*p == '~') {
                        invert = true;
                        p++;
                }

                r = capability_set_from_string(p, &sum);
                if (r < 0)
                        return log_error_errno(r, "Failed to parse %s value %s: %m", field, eq);

                sum = invert ? ~sum : sum;

                r = sd_bus_message_append(m, "(sv)", field, "t", sum);
                if (r < 0)
                        return bus_log_create_error(r);

                return 1;
        }

        if (streq(field, "CPUAffinity")) {
                _cleanup_cpu_free_ cpu_set_t *cpuset = NULL;

                r = parse_cpu_set(eq, &cpuset);
                if (r < 0)
                        return log_error_errno(r, "Failed to parse %s value: %s", field, eq);

                return bus_append_byte_array(m, field, cpuset, CPU_ALLOC_SIZE(r));
        }

        if (STR_IN_SET(field, "RestrictAddressFamilies", "SystemCallFilter")) {
                int whitelist = 1;
                const char *p = eq;

                if (*p == '~') {
                        whitelist = 0;
                        p++;
                }

                r = sd_bus_message_open_container(m, SD_BUS_TYPE_STRUCT, "sv");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_append_basic(m, SD_BUS_TYPE_STRING, field);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_open_container(m, 'v', "(bas)");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_open_container(m, 'r', "bas");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_append_basic(m, 'b', &whitelist);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_open_container(m, 'a', "s");
                if (r < 0)
                        return bus_log_create_error(r);

                for (;;) {
                        _cleanup_free_ char *word = NULL;

                        r = extract_first_word(&p, &word, NULL, EXTRACT_QUOTES);
                        if (r == 0)
                                break;
                        if (r == -ENOMEM)
                                return log_oom();
                        if (r < 0)
                                return log_error_errno(r, "Invalid syntax: %s", eq);

                        r = sd_bus_message_append_basic(m, 's', word);
                        if (r < 0)
                                return bus_log_create_error(r);
                }

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                return 1;
        }

        if (streq(field, "RestrictNamespaces")) {
                bool invert = false;
                unsigned long flags;

                r = parse_boolean(eq);
                if (r > 0)
                        flags = 0;
                else if (r == 0)
                        flags = NAMESPACE_FLAGS_ALL;
                else {
                        if (eq[0] == '~') {
                                invert = true;
                                eq++;
                        }

                        r = namespace_flags_from_string(eq, &flags);
                        if (r < 0)
                                return log_error_errno(r, "Failed to parse %s value %s.", field, eq);
                }

                if (invert)
                        flags = (~flags) & NAMESPACE_FLAGS_ALL;

                r = sd_bus_message_append(m, "(sv)", field, "t", (uint64_t) flags);
                if (r < 0)
                        return bus_log_create_error(r);

                return 1;
        }

        if (STR_IN_SET(field, "BindPaths", "BindReadOnlyPaths")) {
                const char *p = eq;

                r = sd_bus_message_open_container(m, SD_BUS_TYPE_STRUCT, "sv");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_append_basic(m, SD_BUS_TYPE_STRING, field);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_open_container(m, 'v', "a(ssbt)");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_open_container(m, 'a', "(ssbt)");
                if (r < 0)
                        return bus_log_create_error(r);

                for (;;) {
                        _cleanup_free_ char *source = NULL, *destination = NULL;
                        char *s = NULL, *d = NULL;
                        bool ignore_enoent = false;
                        uint64_t flags = MS_REC;

                        r = extract_first_word(&p, &source, ":" WHITESPACE, EXTRACT_QUOTES|EXTRACT_DONT_COALESCE_SEPARATORS);
                        if (r < 0)
                                return log_error_errno(r, "Failed to parse argument: %m");
                        if (r == 0)
                                break;

                        s = source;
                        if (s[0] == '-') {
                                ignore_enoent = true;
                                s++;
                        }

                        if (p && p[-1] == ':') {
                                r = extract_first_word(&p, &destination, ":" WHITESPACE, EXTRACT_QUOTES|EXTRACT_DONT_COALESCE_SEPARATORS);
                                if (r < 0)
                                        return log_error_errno(r, "Failed to parse argument: %m");
                                if (r == 0)
                                        return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                                               "Missing argument after ':': %s",
                                                               eq);

                                d = destination;

                                if (p && p[-1] == ':') {
                                        _cleanup_free_ char *options = NULL;

                                        r = extract_first_word(&p, &options, NULL, EXTRACT_QUOTES);
                                        if (r < 0)
                                                return log_error_errno(r, "Failed to parse argument: %m");

                                        if (isempty(options) || streq(options, "rbind"))
                                                flags = MS_REC;
                                        else if (streq(options, "norbind"))
                                                flags = 0;
                                        else
                                                return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                                                       "Unknown options: %s",
                                                                       eq);
                                }
                        } else
                                d = s;

                        r = sd_bus_message_append(m, "(ssbt)", s, d, ignore_enoent, flags);
                        if (r < 0)
                                return bus_log_create_error(r);
                }

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                return 1;
        }

        if (streq(field, "TemporaryFileSystem")) {
                const char *p = eq;

                r = sd_bus_message_open_container(m, SD_BUS_TYPE_STRUCT, "sv");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_append_basic(m, SD_BUS_TYPE_STRING, field);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_open_container(m, 'v', "a(ss)");
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_open_container(m, 'a', "(ss)");
                if (r < 0)
                        return bus_log_create_error(r);

                for (;;) {
                        _cleanup_free_ char *word = NULL, *path = NULL;
                        const char *w;

                        r = extract_first_word(&p, &word, NULL, EXTRACT_QUOTES);
                        if (r < 0)
                                return log_error_errno(r, "Failed to parse argument: %m");
                        if (r == 0)
                                break;

                        w = word;
                        r = extract_first_word(&w, &path, ":", EXTRACT_DONT_COALESCE_SEPARATORS);
                        if (r < 0)
                                return log_error_errno(r, "Failed to parse argument: %m");
                        if (r == 0)
                                return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                                       "Failed to parse argument: %s",
                                                       p);

                        r = sd_bus_message_append(m, "(ss)", path, w);
                        if (r < 0)
                                return bus_log_create_error(r);
                }

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                r = sd_bus_message_close_container(m);
                if (r < 0)
                        return bus_log_create_error(r);

                return 1;
        }

        return 0;
}