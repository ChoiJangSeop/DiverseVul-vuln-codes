static int automount_dispatch_io(sd_event_source *s, int fd, uint32_t events, void *userdata) {
        _cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
        union autofs_v5_packet_union packet;
        Automount *a = AUTOMOUNT(userdata);
        struct stat st;
        Unit *trigger;
        int r;

        assert(a);
        assert(fd == a->pipe_fd);

        if (events != EPOLLIN) {
                log_unit_error(UNIT(a), "Got invalid poll event %"PRIu32" on pipe (fd=%d)", events, fd);
                goto fail;
        }

        r = loop_read_exact(a->pipe_fd, &packet, sizeof(packet), true);
        if (r < 0) {
                log_unit_error_errno(UNIT(a), r, "Invalid read from pipe: %m");
                goto fail;
        }

        switch (packet.hdr.type) {

        case autofs_ptype_missing_direct:

                if (packet.v5_packet.pid > 0) {
                        _cleanup_free_ char *p = NULL;

                        get_process_comm(packet.v5_packet.pid, &p);
                        log_unit_info(UNIT(a), "Got automount request for %s, triggered by %"PRIu32" (%s)", a->where, packet.v5_packet.pid, strna(p));
                } else
                        log_unit_debug(UNIT(a), "Got direct mount request on %s", a->where);

                r = set_ensure_allocated(&a->tokens, NULL);
                if (r < 0) {
                        log_unit_error(UNIT(a), "Failed to allocate token set.");
                        goto fail;
                }

                r = set_put(a->tokens, UINT_TO_PTR(packet.v5_packet.wait_queue_token));
                if (r < 0) {
                        log_unit_error_errno(UNIT(a), r, "Failed to remember token: %m");
                        goto fail;
                }

                automount_enter_runnning(a);
                break;

        case autofs_ptype_expire_direct:
                log_unit_debug(UNIT(a), "Got direct umount request on %s", a->where);

                automount_stop_expire(a);

                r = set_ensure_allocated(&a->expire_tokens, NULL);
                if (r < 0) {
                        log_unit_error(UNIT(a), "Failed to allocate token set.");
                        goto fail;
                }

                r = set_put(a->expire_tokens, UINT_TO_PTR(packet.v5_packet.wait_queue_token));
                if (r < 0) {
                        log_unit_error_errno(UNIT(a), r, "Failed to remember token: %m");
                        goto fail;
                }

                /* Before we do anything, let's see if somebody is playing games with us? */
                if (lstat(a->where, &st) < 0) {
                        log_unit_warning_errno(UNIT(a), errno, "Failed to stat automount point: %m");
                        goto fail;
                }

                if (!S_ISDIR(st.st_mode) || st.st_dev == a->dev_id) {
                        log_unit_info(UNIT(a), "Automount point already unmounted?");
                        automount_send_ready(a, a->expire_tokens, 0);
                        break;
                }

                trigger = UNIT_TRIGGER(UNIT(a));
                if (!trigger) {
                        log_unit_error(UNIT(a), "Unit to trigger vanished.");
                        goto fail;
                }

                r = manager_add_job(UNIT(a)->manager, JOB_STOP, trigger, JOB_REPLACE, &error, NULL);
                if (r < 0) {
                        log_unit_warning(UNIT(a), "Failed to queue umount startup job: %s", bus_error_message(&error, r));
                        goto fail;
                }
                break;

        default:
                log_unit_error(UNIT(a), "Received unknown automount request %i", packet.hdr.type);
                break;
        }

        return 0;

fail:
        automount_enter_dead(a, AUTOMOUNT_FAILURE_RESOURCES);
        return 0;
}