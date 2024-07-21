void exec_context_dump(const ExecContext *c, FILE* f, const char *prefix) {
        ExecDirectoryType dt;
        char **e, **d;
        unsigned i;
        int r;

        assert(c);
        assert(f);

        prefix = strempty(prefix);

        fprintf(f,
                "%sUMask: %04o\n"
                "%sWorkingDirectory: %s\n"
                "%sRootDirectory: %s\n"
                "%sNonBlocking: %s\n"
                "%sPrivateTmp: %s\n"
                "%sPrivateDevices: %s\n"
                "%sProtectKernelTunables: %s\n"
                "%sProtectKernelModules: %s\n"
                "%sProtectControlGroups: %s\n"
                "%sPrivateNetwork: %s\n"
                "%sPrivateUsers: %s\n"
                "%sProtectHome: %s\n"
                "%sProtectSystem: %s\n"
                "%sMountAPIVFS: %s\n"
                "%sIgnoreSIGPIPE: %s\n"
                "%sMemoryDenyWriteExecute: %s\n"
                "%sRestrictRealtime: %s\n"
                "%sKeyringMode: %s\n"
                "%sProtectHostname: %s\n",
                prefix, c->umask,
                prefix, c->working_directory ? c->working_directory : "/",
                prefix, c->root_directory ? c->root_directory : "/",
                prefix, yes_no(c->non_blocking),
                prefix, yes_no(c->private_tmp),
                prefix, yes_no(c->private_devices),
                prefix, yes_no(c->protect_kernel_tunables),
                prefix, yes_no(c->protect_kernel_modules),
                prefix, yes_no(c->protect_control_groups),
                prefix, yes_no(c->private_network),
                prefix, yes_no(c->private_users),
                prefix, protect_home_to_string(c->protect_home),
                prefix, protect_system_to_string(c->protect_system),
                prefix, yes_no(c->mount_apivfs),
                prefix, yes_no(c->ignore_sigpipe),
                prefix, yes_no(c->memory_deny_write_execute),
                prefix, yes_no(c->restrict_realtime),
                prefix, exec_keyring_mode_to_string(c->keyring_mode),
                prefix, yes_no(c->protect_hostname));

        if (c->root_image)
                fprintf(f, "%sRootImage: %s\n", prefix, c->root_image);

        STRV_FOREACH(e, c->environment)
                fprintf(f, "%sEnvironment: %s\n", prefix, *e);

        STRV_FOREACH(e, c->environment_files)
                fprintf(f, "%sEnvironmentFile: %s\n", prefix, *e);

        STRV_FOREACH(e, c->pass_environment)
                fprintf(f, "%sPassEnvironment: %s\n", prefix, *e);

        STRV_FOREACH(e, c->unset_environment)
                fprintf(f, "%sUnsetEnvironment: %s\n", prefix, *e);

        fprintf(f, "%sRuntimeDirectoryPreserve: %s\n", prefix, exec_preserve_mode_to_string(c->runtime_directory_preserve_mode));

        for (dt = 0; dt < _EXEC_DIRECTORY_TYPE_MAX; dt++) {
                fprintf(f, "%s%sMode: %04o\n", prefix, exec_directory_type_to_string(dt), c->directories[dt].mode);

                STRV_FOREACH(d, c->directories[dt].paths)
                        fprintf(f, "%s%s: %s\n", prefix, exec_directory_type_to_string(dt), *d);
        }

        if (c->nice_set)
                fprintf(f,
                        "%sNice: %i\n",
                        prefix, c->nice);

        if (c->oom_score_adjust_set)
                fprintf(f,
                        "%sOOMScoreAdjust: %i\n",
                        prefix, c->oom_score_adjust);

        for (i = 0; i < RLIM_NLIMITS; i++)
                if (c->rlimit[i]) {
                        fprintf(f, "%sLimit%s: " RLIM_FMT "\n",
                                prefix, rlimit_to_string(i), c->rlimit[i]->rlim_max);
                        fprintf(f, "%sLimit%sSoft: " RLIM_FMT "\n",
                                prefix, rlimit_to_string(i), c->rlimit[i]->rlim_cur);
                }

        if (c->ioprio_set) {
                _cleanup_free_ char *class_str = NULL;

                r = ioprio_class_to_string_alloc(IOPRIO_PRIO_CLASS(c->ioprio), &class_str);
                if (r >= 0)
                        fprintf(f, "%sIOSchedulingClass: %s\n", prefix, class_str);

                fprintf(f, "%sIOPriority: %lu\n", prefix, IOPRIO_PRIO_DATA(c->ioprio));
        }

        if (c->cpu_sched_set) {
                _cleanup_free_ char *policy_str = NULL;

                r = sched_policy_to_string_alloc(c->cpu_sched_policy, &policy_str);
                if (r >= 0)
                        fprintf(f, "%sCPUSchedulingPolicy: %s\n", prefix, policy_str);

                fprintf(f,
                        "%sCPUSchedulingPriority: %i\n"
                        "%sCPUSchedulingResetOnFork: %s\n",
                        prefix, c->cpu_sched_priority,
                        prefix, yes_no(c->cpu_sched_reset_on_fork));
        }

        if (c->cpuset) {
                fprintf(f, "%sCPUAffinity:", prefix);
                for (i = 0; i < c->cpuset_ncpus; i++)
                        if (CPU_ISSET_S(i, CPU_ALLOC_SIZE(c->cpuset_ncpus), c->cpuset))
                                fprintf(f, " %u", i);
                fputs("\n", f);
        }

        if (c->timer_slack_nsec != NSEC_INFINITY)
                fprintf(f, "%sTimerSlackNSec: "NSEC_FMT "\n", prefix, c->timer_slack_nsec);

        fprintf(f,
                "%sStandardInput: %s\n"
                "%sStandardOutput: %s\n"
                "%sStandardError: %s\n",
                prefix, exec_input_to_string(c->std_input),
                prefix, exec_output_to_string(c->std_output),
                prefix, exec_output_to_string(c->std_error));

        if (c->std_input == EXEC_INPUT_NAMED_FD)
                fprintf(f, "%sStandardInputFileDescriptorName: %s\n", prefix, c->stdio_fdname[STDIN_FILENO]);
        if (c->std_output == EXEC_OUTPUT_NAMED_FD)
                fprintf(f, "%sStandardOutputFileDescriptorName: %s\n", prefix, c->stdio_fdname[STDOUT_FILENO]);
        if (c->std_error == EXEC_OUTPUT_NAMED_FD)
                fprintf(f, "%sStandardErrorFileDescriptorName: %s\n", prefix, c->stdio_fdname[STDERR_FILENO]);

        if (c->std_input == EXEC_INPUT_FILE)
                fprintf(f, "%sStandardInputFile: %s\n", prefix, c->stdio_file[STDIN_FILENO]);
        if (c->std_output == EXEC_OUTPUT_FILE)
                fprintf(f, "%sStandardOutputFile: %s\n", prefix, c->stdio_file[STDOUT_FILENO]);
        if (c->std_output == EXEC_OUTPUT_FILE_APPEND)
                fprintf(f, "%sStandardOutputFileToAppend: %s\n", prefix, c->stdio_file[STDOUT_FILENO]);
        if (c->std_error == EXEC_OUTPUT_FILE)
                fprintf(f, "%sStandardErrorFile: %s\n", prefix, c->stdio_file[STDERR_FILENO]);
        if (c->std_error == EXEC_OUTPUT_FILE_APPEND)
                fprintf(f, "%sStandardErrorFileToAppend: %s\n", prefix, c->stdio_file[STDERR_FILENO]);

        if (c->tty_path)
                fprintf(f,
                        "%sTTYPath: %s\n"
                        "%sTTYReset: %s\n"
                        "%sTTYVHangup: %s\n"
                        "%sTTYVTDisallocate: %s\n",
                        prefix, c->tty_path,
                        prefix, yes_no(c->tty_reset),
                        prefix, yes_no(c->tty_vhangup),
                        prefix, yes_no(c->tty_vt_disallocate));

        if (IN_SET(c->std_output,
                   EXEC_OUTPUT_SYSLOG,
                   EXEC_OUTPUT_KMSG,
                   EXEC_OUTPUT_JOURNAL,
                   EXEC_OUTPUT_SYSLOG_AND_CONSOLE,
                   EXEC_OUTPUT_KMSG_AND_CONSOLE,
                   EXEC_OUTPUT_JOURNAL_AND_CONSOLE) ||
            IN_SET(c->std_error,
                   EXEC_OUTPUT_SYSLOG,
                   EXEC_OUTPUT_KMSG,
                   EXEC_OUTPUT_JOURNAL,
                   EXEC_OUTPUT_SYSLOG_AND_CONSOLE,
                   EXEC_OUTPUT_KMSG_AND_CONSOLE,
                   EXEC_OUTPUT_JOURNAL_AND_CONSOLE)) {

                _cleanup_free_ char *fac_str = NULL, *lvl_str = NULL;

                r = log_facility_unshifted_to_string_alloc(c->syslog_priority >> 3, &fac_str);
                if (r >= 0)
                        fprintf(f, "%sSyslogFacility: %s\n", prefix, fac_str);

                r = log_level_to_string_alloc(LOG_PRI(c->syslog_priority), &lvl_str);
                if (r >= 0)
                        fprintf(f, "%sSyslogLevel: %s\n", prefix, lvl_str);
        }

        if (c->log_level_max >= 0) {
                _cleanup_free_ char *t = NULL;

                (void) log_level_to_string_alloc(c->log_level_max, &t);

                fprintf(f, "%sLogLevelMax: %s\n", prefix, strna(t));
        }

        if (c->log_rate_limit_interval_usec > 0) {
                char buf_timespan[FORMAT_TIMESPAN_MAX];

                fprintf(f,
                        "%sLogRateLimitIntervalSec: %s\n",
                        prefix, format_timespan(buf_timespan, sizeof(buf_timespan), c->log_rate_limit_interval_usec, USEC_PER_SEC));
        }

        if (c->log_rate_limit_burst > 0)
                fprintf(f, "%sLogRateLimitBurst: %u\n", prefix, c->log_rate_limit_burst);

        if (c->n_log_extra_fields > 0) {
                size_t j;

                for (j = 0; j < c->n_log_extra_fields; j++) {
                        fprintf(f, "%sLogExtraFields: ", prefix);
                        fwrite(c->log_extra_fields[j].iov_base,
                               1, c->log_extra_fields[j].iov_len,
                               f);
                        fputc('\n', f);
                }
        }

        if (c->secure_bits) {
                _cleanup_free_ char *str = NULL;

                r = secure_bits_to_string_alloc(c->secure_bits, &str);
                if (r >= 0)
                        fprintf(f, "%sSecure Bits: %s\n", prefix, str);
        }

        if (c->capability_bounding_set != CAP_ALL) {
                _cleanup_free_ char *str = NULL;

                r = capability_set_to_string_alloc(c->capability_bounding_set, &str);
                if (r >= 0)
                        fprintf(f, "%sCapabilityBoundingSet: %s\n", prefix, str);
        }

        if (c->capability_ambient_set != 0) {
                _cleanup_free_ char *str = NULL;

                r = capability_set_to_string_alloc(c->capability_ambient_set, &str);
                if (r >= 0)
                        fprintf(f, "%sAmbientCapabilities: %s\n", prefix, str);
        }

        if (c->user)
                fprintf(f, "%sUser: %s\n", prefix, c->user);
        if (c->group)
                fprintf(f, "%sGroup: %s\n", prefix, c->group);

        fprintf(f, "%sDynamicUser: %s\n", prefix, yes_no(c->dynamic_user));

        if (!strv_isempty(c->supplementary_groups)) {
                fprintf(f, "%sSupplementaryGroups:", prefix);
                strv_fprintf(f, c->supplementary_groups);
                fputs("\n", f);
        }

        if (c->pam_name)
                fprintf(f, "%sPAMName: %s\n", prefix, c->pam_name);

        if (!strv_isempty(c->read_write_paths)) {
                fprintf(f, "%sReadWritePaths:", prefix);
                strv_fprintf(f, c->read_write_paths);
                fputs("\n", f);
        }

        if (!strv_isempty(c->read_only_paths)) {
                fprintf(f, "%sReadOnlyPaths:", prefix);
                strv_fprintf(f, c->read_only_paths);
                fputs("\n", f);
        }

        if (!strv_isempty(c->inaccessible_paths)) {
                fprintf(f, "%sInaccessiblePaths:", prefix);
                strv_fprintf(f, c->inaccessible_paths);
                fputs("\n", f);
        }

        if (c->n_bind_mounts > 0)
                for (i = 0; i < c->n_bind_mounts; i++)
                        fprintf(f, "%s%s: %s%s:%s:%s\n", prefix,
                                c->bind_mounts[i].read_only ? "BindReadOnlyPaths" : "BindPaths",
                                c->bind_mounts[i].ignore_enoent ? "-": "",
                                c->bind_mounts[i].source,
                                c->bind_mounts[i].destination,
                                c->bind_mounts[i].recursive ? "rbind" : "norbind");

        if (c->n_temporary_filesystems > 0)
                for (i = 0; i < c->n_temporary_filesystems; i++) {
                        TemporaryFileSystem *t = c->temporary_filesystems + i;

                        fprintf(f, "%sTemporaryFileSystem: %s%s%s\n", prefix,
                                t->path,
                                isempty(t->options) ? "" : ":",
                                strempty(t->options));
                }

        if (c->utmp_id)
                fprintf(f,
                        "%sUtmpIdentifier: %s\n",
                        prefix, c->utmp_id);

        if (c->selinux_context)
                fprintf(f,
                        "%sSELinuxContext: %s%s\n",
                        prefix, c->selinux_context_ignore ? "-" : "", c->selinux_context);

        if (c->apparmor_profile)
                fprintf(f,
                        "%sAppArmorProfile: %s%s\n",
                        prefix, c->apparmor_profile_ignore ? "-" : "", c->apparmor_profile);

        if (c->smack_process_label)
                fprintf(f,
                        "%sSmackProcessLabel: %s%s\n",
                        prefix, c->smack_process_label_ignore ? "-" : "", c->smack_process_label);

        if (c->personality != PERSONALITY_INVALID)
                fprintf(f,
                        "%sPersonality: %s\n",
                        prefix, strna(personality_to_string(c->personality)));

        fprintf(f,
                "%sLockPersonality: %s\n",
                prefix, yes_no(c->lock_personality));

        if (c->syscall_filter) {
#if HAVE_SECCOMP
                Iterator j;
                void *id, *val;
                bool first = true;
#endif

                fprintf(f,
                        "%sSystemCallFilter: ",
                        prefix);

                if (!c->syscall_whitelist)
                        fputc('~', f);

#if HAVE_SECCOMP
                HASHMAP_FOREACH_KEY(val, id, c->syscall_filter, j) {
                        _cleanup_free_ char *name = NULL;
                        const char *errno_name = NULL;
                        int num = PTR_TO_INT(val);

                        if (first)
                                first = false;
                        else
                                fputc(' ', f);

                        name = seccomp_syscall_resolve_num_arch(SCMP_ARCH_NATIVE, PTR_TO_INT(id) - 1);
                        fputs(strna(name), f);

                        if (num >= 0) {
                                errno_name = errno_to_name(num);
                                if (errno_name)
                                        fprintf(f, ":%s", errno_name);
                                else
                                        fprintf(f, ":%d", num);
                        }
                }
#endif

                fputc('\n', f);
        }

        if (c->syscall_archs) {
#if HAVE_SECCOMP
                Iterator j;
                void *id;
#endif

                fprintf(f,
                        "%sSystemCallArchitectures:",
                        prefix);

#if HAVE_SECCOMP
                SET_FOREACH(id, c->syscall_archs, j)
                        fprintf(f, " %s", strna(seccomp_arch_to_string(PTR_TO_UINT32(id) - 1)));
#endif
                fputc('\n', f);
        }

        if (exec_context_restrict_namespaces_set(c)) {
                _cleanup_free_ char *s = NULL;

                r = namespace_flags_to_string(c->restrict_namespaces, &s);
                if (r >= 0)
                        fprintf(f, "%sRestrictNamespaces: %s\n",
                                prefix, s);
        }

        if (c->network_namespace_path)
                fprintf(f,
                        "%sNetworkNamespacePath: %s\n",
                        prefix, c->network_namespace_path);

        if (c->syscall_errno > 0) {
                const char *errno_name;

                fprintf(f, "%sSystemCallErrorNumber: ", prefix);

                errno_name = errno_to_name(c->syscall_errno);
                if (errno_name)
                        fprintf(f, "%s\n", errno_name);
                else
                        fprintf(f, "%d\n", c->syscall_errno);
        }
}