libxlDomainEventHandler(void *data, libxl_event *event)
{
    libxlDriverPrivate *driver = data;
    libxl_shutdown_reason xl_reason = event->u.domain_shutdown.shutdown_reason;
    virDomainObj *vm = NULL;
    g_autoptr(libxlDriverConfig) cfg = NULL;

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

    if (event->type == LIBXL_EVENT_TYPE_DOMAIN_SHUTDOWN) {
        struct libxlEventHandlerThreadInfo *shutdown_info = NULL;
        virThread thread;
        g_autofree char *name = NULL;

        /*
         * Start a thread to handle shutdown.  We don't want to be tying up
         * libxl's event machinery by doing a potentially lengthy shutdown.
         */
        shutdown_info = g_new0(struct libxlEventHandlerThreadInfo, 1);

        shutdown_info->driver = driver;
        shutdown_info->vm = vm;
        shutdown_info->event = (libxl_event *)event;
        name = g_strdup_printf("ev-%d", event->domid);
        /*
         * Cleanup will be handled by the shutdown thread.
         */
        if (virThreadCreateFull(&thread, false, libxlDomainShutdownThread,
                                name, false, shutdown_info) < 0) {
            /*
             * Not much we can do on error here except log it.
             */
            VIR_ERROR(_("Failed to create thread to handle domain shutdown"));
            VIR_FREE(shutdown_info);
            goto cleanup;
        }
        /*
         * virDomainObjEndAPI is called in the shutdown thread, where
         * libxlEventHandlerThreadInfo and libxl_event are also freed.
         */
        return;
    } else if (event->type == LIBXL_EVENT_TYPE_DOMAIN_DEATH) {
        /*
         * On death the domain is cleaned up from Xen's perspective.
         * Cleanup on the libvirt side can be done synchronously.
         */
        libxlDomainHandleDeath(driver, vm);
    }

 cleanup:
    virDomainObjEndAPI(&vm);
    cfg = libxlDriverConfigGet(driver);
    /* Cast away any const */
    libxl_event_free(cfg->ctx, (libxl_event *)event);
}