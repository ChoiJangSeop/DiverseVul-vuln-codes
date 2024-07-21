libxlDomainEventHandler(void *data, libxl_event *event)
{
    libxlDriverPrivate *driver = data;
    libxl_shutdown_reason xl_reason = event->u.domain_shutdown.shutdown_reason;
    virDomainObj *vm = NULL;
    g_autoptr(libxlDriverConfig) cfg = NULL;
    struct libxlEventHandlerThreadInfo *thread_info = NULL;
    virThread thread;
    g_autofree char *thread_name = NULL;

    VIR_DEBUG("Received libxl event '%d' for domid '%d'", event->type, event->domid);

    if (event->type != LIBXL_EVENT_TYPE_DOMAIN_SHUTDOWN &&
            event->type != LIBXL_EVENT_TYPE_DOMAIN_DEATH) {
        VIR_INFO("Unhandled event type %d", event->type);
        goto cleanup;
    }

    /*
     * Similar to the xl implementation, ignore SUSPEND.  Any actions needed
     * after calling libxl_domain_suspend() are handled by its callers.
     */
    if (xl_reason == LIBXL_SHUTDOWN_REASON_SUSPEND)
        goto cleanup;

    vm = virDomainObjListFindByID(driver->domains, event->domid);
    if (!vm) {
        /* Nothing to do if we can't find the virDomainObj */
        goto cleanup;
    }

    /*
     * Start event-specific threads to handle shutdown and death.
     * They are potentially lengthy operations and we don't want to be
     * blocking this event handler while they are in progress.
     */
    if (event->type == LIBXL_EVENT_TYPE_DOMAIN_SHUTDOWN) {
        thread_info = g_new0(struct libxlEventHandlerThreadInfo, 1);

        thread_info->driver = driver;
        thread_info->vm = vm;
        thread_info->event = (libxl_event *)event;
        thread_name = g_strdup_printf("shutdown-event-%d", event->domid);
        /*
         * Cleanup will be handled by the shutdown thread.
         */
        if (virThreadCreateFull(&thread, false, libxlDomainShutdownThread,
                                thread_name, false, thread_info) < 0) {
            /*
             * Not much we can do on error here except log it.
             */
            VIR_ERROR(_("Failed to create thread to handle domain shutdown"));
            goto cleanup;
        }
        /*
         * virDomainObjEndAPI is called in the shutdown thread, where
         * libxlEventHandlerThreadInfo and libxl_event are also freed.
         */
        return;
    } else if (event->type == LIBXL_EVENT_TYPE_DOMAIN_DEATH) {
        thread_info = g_new0(struct libxlEventHandlerThreadInfo, 1);

        thread_info->driver = driver;
        thread_info->vm = vm;
        thread_info->event = (libxl_event *)event;
        thread_name = g_strdup_printf("death-event-%d", event->domid);
        /*
         * Cleanup will be handled by the death thread.
         */
        if (virThreadCreateFull(&thread, false, libxlDomainDeathThread,
                                thread_name, false, thread_info) < 0) {
            /*
             * Not much we can do on error here except log it.
             */
            VIR_ERROR(_("Failed to create thread to handle domain death"));
            goto cleanup;
        }
        /*
         * virDomainObjEndAPI is called in the death thread, where
         * libxlEventHandlerThreadInfo and libxl_event are also freed.
         */
        return;
    }

 cleanup:
    virDomainObjEndAPI(&vm);
    VIR_FREE(thread_info);
    cfg = libxlDriverConfigGet(driver);
    /* Cast away any const */
    libxl_event_free(cfg->ctx, (libxl_event *)event);
}