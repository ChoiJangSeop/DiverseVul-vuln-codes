static void automount_enter_runnning(Automount *a) {
        _cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
        struct stat st;
        int r;

        assert(a);

        /* If the user masked our unit in the meantime, fail */
        if (UNIT(a)->load_state != UNIT_LOADED) {
                log_unit_error(UNIT(a), "Suppressing automount event since unit is no longer loaded.");
                goto fail;
        }

        /* We don't take mount requests anymore if we are supposed to
         * shut down anyway */
        if (unit_stop_pending(UNIT(a))) {
                log_unit_debug(UNIT(a), "Suppressing automount request since unit stop is scheduled.");
                automount_send_ready(a, a->tokens, -EHOSTDOWN);
                automount_send_ready(a, a->expire_tokens, -EHOSTDOWN);
                return;
        }

        mkdir_p_label(a->where, a->directory_mode);

        /* Before we do anything, let's see if somebody is playing games with us? */
        if (lstat(a->where, &st) < 0) {
                log_unit_warning_errno(UNIT(a), errno, "Failed to stat automount point: %m");
                goto fail;
        }

        if (!S_ISDIR(st.st_mode) || st.st_dev != a->dev_id)
                log_unit_info(UNIT(a), "Automount point already active?");
        else {
                Unit *trigger;

                trigger = UNIT_TRIGGER(UNIT(a));
                if (!trigger) {
                        log_unit_error(UNIT(a), "Unit to trigger vanished.");
                        goto fail;
                }

                r = manager_add_job(UNIT(a)->manager, JOB_START, trigger, JOB_REPLACE, &error, NULL);
                if (r < 0) {
                        log_unit_warning(UNIT(a), "Failed to queue mount startup job: %s", bus_error_message(&error, r));
                        goto fail;
                }
        }

        automount_set_state(a, AUTOMOUNT_RUNNING);
        return;

fail:
        automount_enter_dead(a, AUTOMOUNT_FAILURE_RESOURCES);
}