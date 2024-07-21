int manager_new(SystemdRunningAs running_as, Manager **_m) {
        Manager *m;
        int r;

        assert(_m);
        assert(running_as >= 0);
        assert(running_as < _SYSTEMD_RUNNING_AS_MAX);

        m = new0(Manager, 1);
        if (!m)
                return -ENOMEM;

#ifdef ENABLE_EFI
        if (detect_container(NULL) <= 0)
                boot_timestamps(&m->userspace_timestamp, &m->firmware_timestamp, &m->loader_timestamp);
#endif

        m->running_as = running_as;
        m->exit_code = _MANAGER_EXIT_CODE_INVALID;

        m->idle_pipe[0] = m->idle_pipe[1] = m->idle_pipe[2] = m->idle_pipe[3] = -1;

        m->pin_cgroupfs_fd = m->notify_fd = m->signal_fd = m->time_change_fd = m->dev_autofs_fd = m->private_listen_fd = m->kdbus_fd = -1;
        m->current_job_id = 1; /* start as id #1, so that we can leave #0 around as "null-like" value */

        r = manager_default_environment(m);
        if (r < 0)
                goto fail;

        r = hashmap_ensure_allocated(&m->units, string_hash_func, string_compare_func);
        if (r < 0)
                goto fail;

        r = hashmap_ensure_allocated(&m->jobs, trivial_hash_func, trivial_compare_func);
        if (r < 0)
                goto fail;

        r = hashmap_ensure_allocated(&m->cgroup_unit, string_hash_func, string_compare_func);
        if (r < 0)
                goto fail;

        r = hashmap_ensure_allocated(&m->watch_pids, trivial_hash_func, trivial_compare_func);
        if (r < 0)
                goto fail;

        r = hashmap_ensure_allocated(&m->watch_bus, string_hash_func, string_compare_func);
        if (r < 0)
                goto fail;

        r = sd_event_default(&m->event);
        if (r < 0)
                goto fail;

        r = sd_event_add_defer(m->event, manager_dispatch_run_queue, m, &m->run_queue_event_source);
        if (r < 0)
                goto fail;

        r = sd_event_source_set_priority(m->run_queue_event_source, SD_EVENT_PRIORITY_IDLE);
        if (r < 0)
                goto fail;

        r = sd_event_source_set_enabled(m->run_queue_event_source, SD_EVENT_OFF);
        if (r < 0)
                goto fail;

        r = manager_setup_signals(m);
        if (r < 0)
                goto fail;

        r = manager_setup_cgroup(m);
        if (r < 0)
                goto fail;

        r = manager_setup_time_change(m);
        if (r < 0)
                goto fail;

        m->udev = udev_new();
        if (!m->udev) {
                r = -ENOMEM;
                goto fail;
        }

        /* Note that we set up neither kdbus, nor the notify fd
         * here. We do that after deserialization, since they might
         * have gotten serialized across the reexec. */

        m->taint_usr = dir_is_empty("/usr") > 0;

        *_m = m;
        return 0;

fail:
        manager_free(m);
        return r;
}