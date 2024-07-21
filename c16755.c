void manager_free(Manager *m) {
        UnitType c;
        int i;

        assert(m);

        manager_clear_jobs_and_units(m);

        for (c = 0; c < _UNIT_TYPE_MAX; c++)
                if (unit_vtable[c]->shutdown)
                        unit_vtable[c]->shutdown(m);

        /* If we reexecute ourselves, we keep the root cgroup
         * around */
        manager_shutdown_cgroup(m, m->exit_code != MANAGER_REEXECUTE);

        manager_undo_generators(m);

        bus_done(m);

        hashmap_free(m->units);
        hashmap_free(m->jobs);
        hashmap_free(m->watch_pids);
        hashmap_free(m->watch_bus);

        sd_event_source_unref(m->signal_event_source);
        sd_event_source_unref(m->notify_event_source);
        sd_event_source_unref(m->time_change_event_source);
        sd_event_source_unref(m->jobs_in_progress_event_source);
        sd_event_source_unref(m->idle_pipe_event_source);
        sd_event_source_unref(m->run_queue_event_source);

        if (m->signal_fd >= 0)
                close_nointr_nofail(m->signal_fd);
        if (m->notify_fd >= 0)
                close_nointr_nofail(m->notify_fd);
        if (m->time_change_fd >= 0)
                close_nointr_nofail(m->time_change_fd);
        if (m->kdbus_fd >= 0)
                close_nointr_nofail(m->kdbus_fd);

        manager_close_idle_pipe(m);

        udev_unref(m->udev);
        sd_event_unref(m->event);

        free(m->notify_socket);

        lookup_paths_free(&m->lookup_paths);
        strv_free(m->environment);

        hashmap_free(m->cgroup_unit);
        set_free_free(m->unit_path_cache);

        free(m->switch_root);
        free(m->switch_root_init);

        for (i = 0; i < RLIMIT_NLIMITS; i++)
                free(m->rlimit[i]);

        assert(hashmap_isempty(m->units_requiring_mounts_for));
        hashmap_free(m->units_requiring_mounts_for);

        free(m);
}