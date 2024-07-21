static int exec_child(
                Unit *unit,
                const ExecCommand *command,
                const ExecContext *context,
                const ExecParameters *params,
                ExecRuntime *runtime,
                DynamicCreds *dcreds,
                int socket_fd,
                int named_iofds[3],
                int *fds,
                size_t n_socket_fds,
                size_t n_storage_fds,
                char **files_env,
                int user_lookup_fd,
                int *exit_status) {

        _cleanup_strv_free_ char **our_env = NULL, **pass_env = NULL, **accum_env = NULL, **replaced_argv = NULL;
        int *fds_with_exec_fd, n_fds_with_exec_fd, r, ngids = 0, exec_fd = -1;
        _cleanup_free_ gid_t *supplementary_gids = NULL;
        const char *username = NULL, *groupname = NULL;
        _cleanup_free_ char *home_buffer = NULL;
        const char *home = NULL, *shell = NULL;
        char **final_argv = NULL;
        dev_t journal_stream_dev = 0;
        ino_t journal_stream_ino = 0;
        bool needs_sandboxing,          /* Do we need to set up full sandboxing? (i.e. all namespacing, all MAC stuff, caps, yadda yadda */
                needs_setuid,           /* Do we need to do the actual setresuid()/setresgid() calls? */
                needs_mount_namespace,  /* Do we need to set up a mount namespace for this kernel? */
                needs_ambient_hack;     /* Do we need to apply the ambient capabilities hack? */
#if HAVE_SELINUX
        _cleanup_free_ char *mac_selinux_context_net = NULL;
        bool use_selinux = false;
#endif
#if ENABLE_SMACK
        bool use_smack = false;
#endif
#if HAVE_APPARMOR
        bool use_apparmor = false;
#endif
        uid_t uid = UID_INVALID;
        gid_t gid = GID_INVALID;
        size_t n_fds;
        ExecDirectoryType dt;
        int secure_bits;

        assert(unit);
        assert(command);
        assert(context);
        assert(params);
        assert(exit_status);

        rename_process_from_path(command->path);

        /* We reset exactly these signals, since they are the
         * only ones we set to SIG_IGN in the main daemon. All
         * others we leave untouched because we set them to
         * SIG_DFL or a valid handler initially, both of which
         * will be demoted to SIG_DFL. */
        (void) default_signals(SIGNALS_CRASH_HANDLER,
                               SIGNALS_IGNORE, -1);

        if (context->ignore_sigpipe)
                (void) ignore_signals(SIGPIPE, -1);

        r = reset_signal_mask();
        if (r < 0) {
                *exit_status = EXIT_SIGNAL_MASK;
                return log_unit_error_errno(unit, r, "Failed to set process signal mask: %m");
        }

        if (params->idle_pipe)
                do_idle_pipe_dance(params->idle_pipe);

        /* Close fds we don't need very early to make sure we don't block init reexecution because it cannot bind its
         * sockets. Among the fds we close are the logging fds, and we want to keep them closed, so that we don't have
         * any fds open we don't really want open during the transition. In order to make logging work, we switch the
         * log subsystem into open_when_needed mode, so that it reopens the logs on every single log call. */

        log_forget_fds();
        log_set_open_when_needed(true);

        /* In case anything used libc syslog(), close this here, too */
        closelog();

        n_fds = n_socket_fds + n_storage_fds;
        r = close_remaining_fds(params, runtime, dcreds, user_lookup_fd, socket_fd, params->exec_fd, fds, n_fds);
        if (r < 0) {
                *exit_status = EXIT_FDS;
                return log_unit_error_errno(unit, r, "Failed to close unwanted file descriptors: %m");
        }

        if (!context->same_pgrp)
                if (setsid() < 0) {
                        *exit_status = EXIT_SETSID;
                        return log_unit_error_errno(unit, errno, "Failed to create new process session: %m");
                }

        exec_context_tty_reset(context, params);

        if (unit_shall_confirm_spawn(unit)) {
                const char *vc = params->confirm_spawn;
                _cleanup_free_ char *cmdline = NULL;

                cmdline = exec_command_line(command->argv);
                if (!cmdline) {
                        *exit_status = EXIT_MEMORY;
                        return log_oom();
                }

                r = ask_for_confirmation(vc, unit, cmdline);
                if (r != CONFIRM_EXECUTE) {
                        if (r == CONFIRM_PRETEND_SUCCESS) {
                                *exit_status = EXIT_SUCCESS;
                                return 0;
                        }
                        *exit_status = EXIT_CONFIRM;
                        log_unit_error(unit, "Execution cancelled by the user");
                        return -ECANCELED;
                }
        }

        /* We are about to invoke NSS and PAM modules. Let's tell them what we are doing here, maybe they care. This is
         * used by nss-resolve to disable itself when we are about to start systemd-resolved, to avoid deadlocks. Note
         * that these env vars do not survive the execve(), which means they really only apply to the PAM and NSS
         * invocations themselves. Also note that while we'll only invoke NSS modules involved in user management they
         * might internally call into other NSS modules that are involved in hostname resolution, we never know. */
        if (setenv("SYSTEMD_ACTIVATION_UNIT", unit->id, true) != 0 ||
            setenv("SYSTEMD_ACTIVATION_SCOPE", MANAGER_IS_SYSTEM(unit->manager) ? "system" : "user", true) != 0) {
                *exit_status = EXIT_MEMORY;
                return log_unit_error_errno(unit, errno, "Failed to update environment: %m");
        }

        if (context->dynamic_user && dcreds) {
                _cleanup_strv_free_ char **suggested_paths = NULL;

                /* On top of that, make sure we bypass our own NSS module nss-systemd comprehensively for any NSS
                 * checks, if DynamicUser=1 is used, as we shouldn't create a feedback loop with ourselves here.*/
                if (putenv((char*) "SYSTEMD_NSS_DYNAMIC_BYPASS=1") != 0) {
                        *exit_status = EXIT_USER;
                        return log_unit_error_errno(unit, errno, "Failed to update environment: %m");
                }

                r = compile_suggested_paths(context, params, &suggested_paths);
                if (r < 0) {
                        *exit_status = EXIT_MEMORY;
                        return log_oom();
                }

                r = dynamic_creds_realize(dcreds, suggested_paths, &uid, &gid);
                if (r < 0) {
                        *exit_status = EXIT_USER;
                        if (r == -EILSEQ) {
                                log_unit_error(unit, "Failed to update dynamic user credentials: User or group with specified name already exists.");
                                return -EOPNOTSUPP;
                        }
                        return log_unit_error_errno(unit, r, "Failed to update dynamic user credentials: %m");
                }

                if (!uid_is_valid(uid)) {
                        *exit_status = EXIT_USER;
                        log_unit_error(unit, "UID validation failed for \""UID_FMT"\"", uid);
                        return -ESRCH;
                }

                if (!gid_is_valid(gid)) {
                        *exit_status = EXIT_USER;
                        log_unit_error(unit, "GID validation failed for \""GID_FMT"\"", gid);
                        return -ESRCH;
                }

                if (dcreds->user)
                        username = dcreds->user->name;

        } else {
                r = get_fixed_user(context, &username, &uid, &gid, &home, &shell);
                if (r < 0) {
                        *exit_status = EXIT_USER;
                        return log_unit_error_errno(unit, r, "Failed to determine user credentials: %m");
                }

                r = get_fixed_group(context, &groupname, &gid);
                if (r < 0) {
                        *exit_status = EXIT_GROUP;
                        return log_unit_error_errno(unit, r, "Failed to determine group credentials: %m");
                }
        }

        /* Initialize user supplementary groups and get SupplementaryGroups= ones */
        r = get_supplementary_groups(context, username, groupname, gid,
                                     &supplementary_gids, &ngids);
        if (r < 0) {
                *exit_status = EXIT_GROUP;
                return log_unit_error_errno(unit, r, "Failed to determine supplementary groups: %m");
        }

        r = send_user_lookup(unit, user_lookup_fd, uid, gid);
        if (r < 0) {
                *exit_status = EXIT_USER;
                return log_unit_error_errno(unit, r, "Failed to send user credentials to PID1: %m");
        }

        user_lookup_fd = safe_close(user_lookup_fd);

        r = acquire_home(context, uid, &home, &home_buffer);
        if (r < 0) {
                *exit_status = EXIT_CHDIR;
                return log_unit_error_errno(unit, r, "Failed to determine $HOME for user: %m");
        }

        /* If a socket is connected to STDIN/STDOUT/STDERR, we
         * must sure to drop O_NONBLOCK */
        if (socket_fd >= 0)
                (void) fd_nonblock(socket_fd, false);

        /* Journald will try to look-up our cgroup in order to populate _SYSTEMD_CGROUP and _SYSTEMD_UNIT fields.
         * Hence we need to migrate to the target cgroup from init.scope before connecting to journald */
        if (params->cgroup_path) {
                _cleanup_free_ char *p = NULL;

                r = exec_parameters_get_cgroup_path(params, &p);
                if (r < 0) {
                        *exit_status = EXIT_CGROUP;
                        return log_unit_error_errno(unit, r, "Failed to acquire cgroup path: %m");
                }

                r = cg_attach_everywhere(params->cgroup_supported, p, 0, NULL, NULL);
                if (r < 0) {
                        *exit_status = EXIT_CGROUP;
                        return log_unit_error_errno(unit, r, "Failed to attach to cgroup %s: %m", p);
                }
        }

        if (context->network_namespace_path && runtime && runtime->netns_storage_socket[0] >= 0) {
                r = open_netns_path(runtime->netns_storage_socket, context->network_namespace_path);
                if (r < 0) {
                        *exit_status = EXIT_NETWORK;
                        return log_unit_error_errno(unit, r, "Failed to open network namespace path %s: %m", context->network_namespace_path);
                }
        }

        r = setup_input(context, params, socket_fd, named_iofds);
        if (r < 0) {
                *exit_status = EXIT_STDIN;
                return log_unit_error_errno(unit, r, "Failed to set up standard input: %m");
        }

        r = setup_output(unit, context, params, STDOUT_FILENO, socket_fd, named_iofds, basename(command->path), uid, gid, &journal_stream_dev, &journal_stream_ino);
        if (r < 0) {
                *exit_status = EXIT_STDOUT;
                return log_unit_error_errno(unit, r, "Failed to set up standard output: %m");
        }

        r = setup_output(unit, context, params, STDERR_FILENO, socket_fd, named_iofds, basename(command->path), uid, gid, &journal_stream_dev, &journal_stream_ino);
        if (r < 0) {
                *exit_status = EXIT_STDERR;
                return log_unit_error_errno(unit, r, "Failed to set up standard error output: %m");
        }

        if (context->oom_score_adjust_set) {
                /* When we can't make this change due to EPERM, then let's silently skip over it. User namespaces
                 * prohibit write access to this file, and we shouldn't trip up over that. */
                r = set_oom_score_adjust(context->oom_score_adjust);
                if (IN_SET(r, -EPERM, -EACCES))
                        log_unit_debug_errno(unit, r, "Failed to adjust OOM setting, assuming containerized execution, ignoring: %m");
                else if (r < 0) {
                        *exit_status = EXIT_OOM_ADJUST;
                        return log_unit_error_errno(unit, r, "Failed to adjust OOM setting: %m");
                }
        }

        if (context->nice_set)
                if (setpriority(PRIO_PROCESS, 0, context->nice) < 0) {
                        *exit_status = EXIT_NICE;
                        return log_unit_error_errno(unit, errno, "Failed to set up process scheduling priority (nice level): %m");
                }

        if (context->cpu_sched_set) {
                struct sched_param param = {
                        .sched_priority = context->cpu_sched_priority,
                };

                r = sched_setscheduler(0,
                                       context->cpu_sched_policy |
                                       (context->cpu_sched_reset_on_fork ?
                                        SCHED_RESET_ON_FORK : 0),
                                       &param);
                if (r < 0) {
                        *exit_status = EXIT_SETSCHEDULER;
                        return log_unit_error_errno(unit, errno, "Failed to set up CPU scheduling: %m");
                }
        }

        if (context->cpuset)
                if (sched_setaffinity(0, CPU_ALLOC_SIZE(context->cpuset_ncpus), context->cpuset) < 0) {
                        *exit_status = EXIT_CPUAFFINITY;
                        return log_unit_error_errno(unit, errno, "Failed to set up CPU affinity: %m");
                }

        if (context->ioprio_set)
                if (ioprio_set(IOPRIO_WHO_PROCESS, 0, context->ioprio) < 0) {
                        *exit_status = EXIT_IOPRIO;
                        return log_unit_error_errno(unit, errno, "Failed to set up IO scheduling priority: %m");
                }

        if (context->timer_slack_nsec != NSEC_INFINITY)
                if (prctl(PR_SET_TIMERSLACK, context->timer_slack_nsec) < 0) {
                        *exit_status = EXIT_TIMERSLACK;
                        return log_unit_error_errno(unit, errno, "Failed to set up timer slack: %m");
                }

        if (context->personality != PERSONALITY_INVALID) {
                r = safe_personality(context->personality);
                if (r < 0) {
                        *exit_status = EXIT_PERSONALITY;
                        return log_unit_error_errno(unit, r, "Failed to set up execution domain (personality): %m");
                }
        }

        if (context->utmp_id)
                utmp_put_init_process(context->utmp_id, getpid_cached(), getsid(0),
                                      context->tty_path,
                                      context->utmp_mode == EXEC_UTMP_INIT  ? INIT_PROCESS :
                                      context->utmp_mode == EXEC_UTMP_LOGIN ? LOGIN_PROCESS :
                                      USER_PROCESS,
                                      username);

        if (uid_is_valid(uid)) {
                r = chown_terminal(STDIN_FILENO, uid);
                if (r < 0) {
                        *exit_status = EXIT_STDIN;
                        return log_unit_error_errno(unit, r, "Failed to change ownership of terminal: %m");
                }
        }

        /* If delegation is enabled we'll pass ownership of the cgroup to the user of the new process. On cgroup v1
         * this is only about systemd's own hierarchy, i.e. not the controller hierarchies, simply because that's not
         * safe. On cgroup v2 there's only one hierarchy anyway, and delegation is safe there, hence in that case only
         * touch a single hierarchy too. */
        if (params->cgroup_path && context->user && (params->flags & EXEC_CGROUP_DELEGATE)) {
                r = cg_set_access(SYSTEMD_CGROUP_CONTROLLER, params->cgroup_path, uid, gid);
                if (r < 0) {
                        *exit_status = EXIT_CGROUP;
                        return log_unit_error_errno(unit, r, "Failed to adjust control group access: %m");
                }
        }

        for (dt = 0; dt < _EXEC_DIRECTORY_TYPE_MAX; dt++) {
                r = setup_exec_directory(context, params, uid, gid, dt, exit_status);
                if (r < 0)
                        return log_unit_error_errno(unit, r, "Failed to set up special execution directory in %s: %m", params->prefix[dt]);
        }

        r = build_environment(
                        unit,
                        context,
                        params,
                        n_fds,
                        home,
                        username,
                        shell,
                        journal_stream_dev,
                        journal_stream_ino,
                        &our_env);
        if (r < 0) {
                *exit_status = EXIT_MEMORY;
                return log_oom();
        }

        r = build_pass_environment(context, &pass_env);
        if (r < 0) {
                *exit_status = EXIT_MEMORY;
                return log_oom();
        }

        accum_env = strv_env_merge(5,
                                   params->environment,
                                   our_env,
                                   pass_env,
                                   context->environment,
                                   files_env,
                                   NULL);
        if (!accum_env) {
                *exit_status = EXIT_MEMORY;
                return log_oom();
        }
        accum_env = strv_env_clean(accum_env);

        (void) umask(context->umask);

        r = setup_keyring(unit, context, params, uid, gid);
        if (r < 0) {
                *exit_status = EXIT_KEYRING;
                return log_unit_error_errno(unit, r, "Failed to set up kernel keyring: %m");
        }

        /* We need sandboxing if the caller asked us to apply it and the command isn't explicitly excepted from it */
        needs_sandboxing = (params->flags & EXEC_APPLY_SANDBOXING) && !(command->flags & EXEC_COMMAND_FULLY_PRIVILEGED);

        /* We need the ambient capability hack, if the caller asked us to apply it and the command is marked for it, and the kernel doesn't actually support ambient caps */
        needs_ambient_hack = (params->flags & EXEC_APPLY_SANDBOXING) && (command->flags & EXEC_COMMAND_AMBIENT_MAGIC) && !ambient_capabilities_supported();

        /* We need setresuid() if the caller asked us to apply sandboxing and the command isn't explicitly excepted from either whole sandboxing or just setresuid() itself, and the ambient hack is not desired */
        if (needs_ambient_hack)
                needs_setuid = false;
        else
                needs_setuid = (params->flags & EXEC_APPLY_SANDBOXING) && !(command->flags & (EXEC_COMMAND_FULLY_PRIVILEGED|EXEC_COMMAND_NO_SETUID));

        if (needs_sandboxing) {
                /* MAC enablement checks need to be done before a new mount ns is created, as they rely on /sys being
                 * present. The actual MAC context application will happen later, as late as possible, to avoid
                 * impacting our own code paths. */

#if HAVE_SELINUX
                use_selinux = mac_selinux_use();
#endif
#if ENABLE_SMACK
                use_smack = mac_smack_use();
#endif
#if HAVE_APPARMOR
                use_apparmor = mac_apparmor_use();
#endif
        }

        if (needs_sandboxing) {
                int which_failed;

                /* Let's set the resource limits before we call into PAM, so that pam_limits wins over what
                 * is set here. (See below.) */

                r = setrlimit_closest_all((const struct rlimit* const *) context->rlimit, &which_failed);
                if (r < 0) {
                        *exit_status = EXIT_LIMITS;
                        return log_unit_error_errno(unit, r, "Failed to adjust resource limit RLIMIT_%s: %m", rlimit_to_string(which_failed));
                }
        }

        if (needs_setuid) {

                /* Let's call into PAM after we set up our own idea of resource limits to that pam_limits
                 * wins here. (See above.) */

                if (context->pam_name && username) {
                        r = setup_pam(context->pam_name, username, uid, gid, context->tty_path, &accum_env, fds, n_fds);
                        if (r < 0) {
                                *exit_status = EXIT_PAM;
                                return log_unit_error_errno(unit, r, "Failed to set up PAM session: %m");
                        }
                }
        }

        if ((context->private_network || context->network_namespace_path) && runtime && runtime->netns_storage_socket[0] >= 0) {

                if (ns_type_supported(NAMESPACE_NET)) {
                        r = setup_netns(runtime->netns_storage_socket);
                        if (r < 0) {
                                *exit_status = EXIT_NETWORK;
                                return log_unit_error_errno(unit, r, "Failed to set up network namespacing: %m");
                        }
                } else if (context->network_namespace_path) {
                        *exit_status = EXIT_NETWORK;
                        return log_unit_error_errno(unit, SYNTHETIC_ERRNO(EOPNOTSUPP), "NetworkNamespacePath= is not supported, refusing.");
                } else
                        log_unit_warning(unit, "PrivateNetwork=yes is configured, but the kernel does not support network namespaces, ignoring.");
        }

        needs_mount_namespace = exec_needs_mount_namespace(context, params, runtime);
        if (needs_mount_namespace) {
                r = apply_mount_namespace(unit, command, context, params, runtime);
                if (r < 0) {
                        *exit_status = EXIT_NAMESPACE;
                        return log_unit_error_errno(unit, r, "Failed to set up mount namespacing: %m");
                }
        }

        if (context->protect_hostname) {
                if (ns_type_supported(NAMESPACE_UTS)) {
                        if (unshare(CLONE_NEWUTS) < 0) {
                                *exit_status = EXIT_NAMESPACE;
                                return log_unit_error_errno(unit, errno, "Failed to set up UTS namespacing: %m");
                        }
                } else
                        log_unit_warning(unit, "ProtectHostname=yes is configured, but the kernel does not support UTS namespaces, ignoring namespace setup.");
#if HAVE_SECCOMP
                r = seccomp_protect_hostname();
                if (r < 0) {
                        *exit_status = EXIT_SECCOMP;
                        return log_unit_error_errno(unit, r, "Failed to apply hostname restrictions: %m");
                }
#endif
        }

        /* Drop groups as early as possbile */
        if (needs_setuid) {
                r = enforce_groups(gid, supplementary_gids, ngids);
                if (r < 0) {
                        *exit_status = EXIT_GROUP;
                        return log_unit_error_errno(unit, r, "Changing group credentials failed: %m");
                }
        }

        if (needs_sandboxing) {
#if HAVE_SELINUX
                if (use_selinux && params->selinux_context_net && socket_fd >= 0) {
                        r = mac_selinux_get_child_mls_label(socket_fd, command->path, context->selinux_context, &mac_selinux_context_net);
                        if (r < 0) {
                                *exit_status = EXIT_SELINUX_CONTEXT;
                                return log_unit_error_errno(unit, r, "Failed to determine SELinux context: %m");
                        }
                }
#endif

                if (context->private_users) {
                        r = setup_private_users(uid, gid);
                        if (r < 0) {
                                *exit_status = EXIT_USER;
                                return log_unit_error_errno(unit, r, "Failed to set up user namespacing: %m");
                        }
                }
        }

        /* We repeat the fd closing here, to make sure that nothing is leaked from the PAM modules. Note that we are
         * more aggressive this time since socket_fd and the netns fds we don't need anymore. We do keep the exec_fd
         * however if we have it as we want to keep it open until the final execve(). */

        if (params->exec_fd >= 0) {
                exec_fd = params->exec_fd;

                if (exec_fd < 3 + (int) n_fds) {
                        int moved_fd;

                        /* Let's move the exec fd far up, so that it's outside of the fd range we want to pass to the
                         * process we are about to execute. */

                        moved_fd = fcntl(exec_fd, F_DUPFD_CLOEXEC, 3 + (int) n_fds);
                        if (moved_fd < 0) {
                                *exit_status = EXIT_FDS;
                                return log_unit_error_errno(unit, errno, "Couldn't move exec fd up: %m");
                        }

                        safe_close(exec_fd);
                        exec_fd = moved_fd;
                } else {
                        /* This fd should be FD_CLOEXEC already, but let's make sure. */
                        r = fd_cloexec(exec_fd, true);
                        if (r < 0) {
                                *exit_status = EXIT_FDS;
                                return log_unit_error_errno(unit, r, "Failed to make exec fd FD_CLOEXEC: %m");
                        }
                }

                fds_with_exec_fd = newa(int, n_fds + 1);
                memcpy_safe(fds_with_exec_fd, fds, n_fds * sizeof(int));
                fds_with_exec_fd[n_fds] = exec_fd;
                n_fds_with_exec_fd = n_fds + 1;
        } else {
                fds_with_exec_fd = fds;
                n_fds_with_exec_fd = n_fds;
        }

        r = close_all_fds(fds_with_exec_fd, n_fds_with_exec_fd);
        if (r >= 0)
                r = shift_fds(fds, n_fds);
        if (r >= 0)
                r = flags_fds(fds, n_socket_fds, n_storage_fds, context->non_blocking);
        if (r < 0) {
                *exit_status = EXIT_FDS;
                return log_unit_error_errno(unit, r, "Failed to adjust passed file descriptors: %m");
        }

        /* At this point, the fds we want to pass to the program are all ready and set up, with O_CLOEXEC turned off
         * and at the right fd numbers. The are no other fds open, with one exception: the exec_fd if it is defined,
         * and it has O_CLOEXEC set, after all we want it to be closed by the execve(), so that our parent knows we
         * came this far. */

        secure_bits = context->secure_bits;

        if (needs_sandboxing) {
                uint64_t bset;

                /* Set the RTPRIO resource limit to 0, but only if nothing else was explicitly
                 * requested. (Note this is placed after the general resource limit initialization, see
                 * above, in order to take precedence.) */
                if (context->restrict_realtime && !context->rlimit[RLIMIT_RTPRIO]) {
                        if (setrlimit(RLIMIT_RTPRIO, &RLIMIT_MAKE_CONST(0)) < 0) {
                                *exit_status = EXIT_LIMITS;
                                return log_unit_error_errno(unit, errno, "Failed to adjust RLIMIT_RTPRIO resource limit: %m");
                        }
                }

#if ENABLE_SMACK
                /* LSM Smack needs the capability CAP_MAC_ADMIN to change the current execution security context of the
                 * process. This is the latest place before dropping capabilities. Other MAC context are set later. */
                if (use_smack) {
                        r = setup_smack(context, command);
                        if (r < 0) {
                                *exit_status = EXIT_SMACK_PROCESS_LABEL;
                                return log_unit_error_errno(unit, r, "Failed to set SMACK process label: %m");
                        }
                }
#endif

                bset = context->capability_bounding_set;
                /* If the ambient caps hack is enabled (which means the kernel can't do them, and the user asked for
                 * our magic fallback), then let's add some extra caps, so that the service can drop privs of its own,
                 * instead of us doing that */
                if (needs_ambient_hack)
                        bset |= (UINT64_C(1) << CAP_SETPCAP) |
                                (UINT64_C(1) << CAP_SETUID) |
                                (UINT64_C(1) << CAP_SETGID);

                if (!cap_test_all(bset)) {
                        r = capability_bounding_set_drop(bset, false);
                        if (r < 0) {
                                *exit_status = EXIT_CAPABILITIES;
                                return log_unit_error_errno(unit, r, "Failed to drop capabilities: %m");
                        }
                }

                /* This is done before enforce_user, but ambient set
                 * does not survive over setresuid() if keep_caps is not set. */
                if (!needs_ambient_hack &&
                    context->capability_ambient_set != 0) {
                        r = capability_ambient_set_apply(context->capability_ambient_set, true);
                        if (r < 0) {
                                *exit_status = EXIT_CAPABILITIES;
                                return log_unit_error_errno(unit, r, "Failed to apply ambient capabilities (before UID change): %m");
                        }
                }
        }

        if (needs_setuid) {
                if (uid_is_valid(uid)) {
                        r = enforce_user(context, uid);
                        if (r < 0) {
                                *exit_status = EXIT_USER;
                                return log_unit_error_errno(unit, r, "Failed to change UID to " UID_FMT ": %m", uid);
                        }

                        if (!needs_ambient_hack &&
                            context->capability_ambient_set != 0) {

                                /* Fix the ambient capabilities after user change. */
                                r = capability_ambient_set_apply(context->capability_ambient_set, false);
                                if (r < 0) {
                                        *exit_status = EXIT_CAPABILITIES;
                                        return log_unit_error_errno(unit, r, "Failed to apply ambient capabilities (after UID change): %m");
                                }

                                /* If we were asked to change user and ambient capabilities
                                 * were requested, we had to add keep-caps to the securebits
                                 * so that we would maintain the inherited capability set
                                 * through the setresuid(). Make sure that the bit is added
                                 * also to the context secure_bits so that we don't try to
                                 * drop the bit away next. */

                                secure_bits |= 1<<SECURE_KEEP_CAPS;
                        }
                }
        }

        /* Apply working directory here, because the working directory might be on NFS and only the user running
         * this service might have the correct privilege to change to the working directory */
        r = apply_working_directory(context, params, home, needs_mount_namespace, exit_status);
        if (r < 0)
                return log_unit_error_errno(unit, r, "Changing to the requested working directory failed: %m");

        if (needs_sandboxing) {
                /* Apply other MAC contexts late, but before seccomp syscall filtering, as those should really be last to
                 * influence our own codepaths as little as possible. Moreover, applying MAC contexts usually requires
                 * syscalls that are subject to seccomp filtering, hence should probably be applied before the syscalls
                 * are restricted. */

#if HAVE_SELINUX
                if (use_selinux) {
                        char *exec_context = mac_selinux_context_net ?: context->selinux_context;

                        if (exec_context) {
                                r = setexeccon(exec_context);
                                if (r < 0) {
                                        *exit_status = EXIT_SELINUX_CONTEXT;
                                        return log_unit_error_errno(unit, r, "Failed to change SELinux context to %s: %m", exec_context);
                                }
                        }
                }
#endif

#if HAVE_APPARMOR
                if (use_apparmor && context->apparmor_profile) {
                        r = aa_change_onexec(context->apparmor_profile);
                        if (r < 0 && !context->apparmor_profile_ignore) {
                                *exit_status = EXIT_APPARMOR_PROFILE;
                                return log_unit_error_errno(unit, errno, "Failed to prepare AppArmor profile change to %s: %m", context->apparmor_profile);
                        }
                }
#endif

                /* PR_GET_SECUREBITS is not privileged, while PR_SET_SECUREBITS is. So to suppress potential EPERMs
                 * we'll try not to call PR_SET_SECUREBITS unless necessary. */
                if (prctl(PR_GET_SECUREBITS) != secure_bits)
                        if (prctl(PR_SET_SECUREBITS, secure_bits) < 0) {
                                *exit_status = EXIT_SECUREBITS;
                                return log_unit_error_errno(unit, errno, "Failed to set process secure bits: %m");
                        }

                if (context_has_no_new_privileges(context))
                        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0) {
                                *exit_status = EXIT_NO_NEW_PRIVILEGES;
                                return log_unit_error_errno(unit, errno, "Failed to disable new privileges: %m");
                        }

#if HAVE_SECCOMP
                r = apply_address_families(unit, context);
                if (r < 0) {
                        *exit_status = EXIT_ADDRESS_FAMILIES;
                        return log_unit_error_errno(unit, r, "Failed to restrict address families: %m");
                }

                r = apply_memory_deny_write_execute(unit, context);
                if (r < 0) {
                        *exit_status = EXIT_SECCOMP;
                        return log_unit_error_errno(unit, r, "Failed to disable writing to executable memory: %m");
                }

                r = apply_restrict_realtime(unit, context);
                if (r < 0) {
                        *exit_status = EXIT_SECCOMP;
                        return log_unit_error_errno(unit, r, "Failed to apply realtime restrictions: %m");
                }

                r = apply_restrict_namespaces(unit, context);
                if (r < 0) {
                        *exit_status = EXIT_SECCOMP;
                        return log_unit_error_errno(unit, r, "Failed to apply namespace restrictions: %m");
                }

                r = apply_protect_sysctl(unit, context);
                if (r < 0) {
                        *exit_status = EXIT_SECCOMP;
                        return log_unit_error_errno(unit, r, "Failed to apply sysctl restrictions: %m");
                }

                r = apply_protect_kernel_modules(unit, context);
                if (r < 0) {
                        *exit_status = EXIT_SECCOMP;
                        return log_unit_error_errno(unit, r, "Failed to apply module loading restrictions: %m");
                }

                r = apply_private_devices(unit, context);
                if (r < 0) {
                        *exit_status = EXIT_SECCOMP;
                        return log_unit_error_errno(unit, r, "Failed to set up private devices: %m");
                }

                r = apply_syscall_archs(unit, context);
                if (r < 0) {
                        *exit_status = EXIT_SECCOMP;
                        return log_unit_error_errno(unit, r, "Failed to apply syscall architecture restrictions: %m");
                }

                r = apply_lock_personality(unit, context);
                if (r < 0) {
                        *exit_status = EXIT_SECCOMP;
                        return log_unit_error_errno(unit, r, "Failed to lock personalities: %m");
                }

                /* This really should remain the last step before the execve(), to make sure our own code is unaffected
                 * by the filter as little as possible. */
                r = apply_syscall_filter(unit, context, needs_ambient_hack);
                if (r < 0) {
                        *exit_status = EXIT_SECCOMP;
                        return log_unit_error_errno(unit, r, "Failed to apply system call filters: %m");
                }
#endif
        }

        if (!strv_isempty(context->unset_environment)) {
                char **ee = NULL;

                ee = strv_env_delete(accum_env, 1, context->unset_environment);
                if (!ee) {
                        *exit_status = EXIT_MEMORY;
                        return log_oom();
                }

                strv_free_and_replace(accum_env, ee);
        }

        if (!FLAGS_SET(command->flags, EXEC_COMMAND_NO_ENV_EXPAND)) {
                replaced_argv = replace_env_argv(command->argv, accum_env);
                if (!replaced_argv) {
                        *exit_status = EXIT_MEMORY;
                        return log_oom();
                }
                final_argv = replaced_argv;
        } else
                final_argv = command->argv;

        if (DEBUG_LOGGING) {
                _cleanup_free_ char *line;

                line = exec_command_line(final_argv);
                if (line)
                        log_struct(LOG_DEBUG,
                                   "EXECUTABLE=%s", command->path,
                                   LOG_UNIT_MESSAGE(unit, "Executing: %s", line),
                                   LOG_UNIT_ID(unit),
                                   LOG_UNIT_INVOCATION_ID(unit));
        }

        if (exec_fd >= 0) {
                uint8_t hot = 1;

                /* We have finished with all our initializations. Let's now let the manager know that. From this point
                 * on, if the manager sees POLLHUP on the exec_fd, then execve() was successful. */

                if (write(exec_fd, &hot, sizeof(hot)) < 0) {
                        *exit_status = EXIT_EXEC;
                        return log_unit_error_errno(unit, errno, "Failed to enable exec_fd: %m");
                }
        }

        execve(command->path, final_argv, accum_env);
        r = -errno;

        if (exec_fd >= 0) {
                uint8_t hot = 0;

                /* The execve() failed. This means the exec_fd is still open. Which means we need to tell the manager
                 * that POLLHUP on it no longer means execve() succeeded. */

                if (write(exec_fd, &hot, sizeof(hot)) < 0) {
                        *exit_status = EXIT_EXEC;
                        return log_unit_error_errno(unit, errno, "Failed to disable exec_fd: %m");
                }
        }

        if (r == -ENOENT && (command->flags & EXEC_COMMAND_IGNORE_FAILURE)) {
                log_struct_errno(LOG_INFO, r,
                                 "MESSAGE_ID=" SD_MESSAGE_SPAWN_FAILED_STR,
                                 LOG_UNIT_ID(unit),
                                 LOG_UNIT_INVOCATION_ID(unit),
                                 LOG_UNIT_MESSAGE(unit, "Executable %s missing, skipping: %m",
                                                  command->path),
                                 "EXECUTABLE=%s", command->path);
                return 0;
        }

        *exit_status = EXIT_EXEC;
        return log_unit_error_errno(unit, r, "Failed to execute command: %m");
}