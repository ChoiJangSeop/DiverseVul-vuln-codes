libxlDomainShutdownThread(void *opaque)
{
    struct libxlShutdownThreadInfo *shutdown_info = opaque;
    virDomainObj *vm = shutdown_info->vm;
    libxl_event *ev = shutdown_info->event;
    libxlDriverPrivate *driver = shutdown_info->driver;
    virObjectEvent *dom_event = NULL;
    libxl_shutdown_reason xl_reason = ev->u.domain_shutdown.shutdown_reason;
    g_autoptr(libxlDriverConfig) cfg = libxlDriverConfigGet(driver);
    libxl_domain_config d_config;

    libxl_domain_config_init(&d_config);

    if (libxlDomainObjBeginJob(driver, vm, LIBXL_JOB_MODIFY) < 0)
        goto cleanup;

    if (xl_reason == LIBXL_SHUTDOWN_REASON_POWEROFF) {
        virDomainObjSetState(vm, VIR_DOMAIN_SHUTOFF,
                             VIR_DOMAIN_SHUTOFF_SHUTDOWN);

        dom_event = virDomainEventLifecycleNewFromObj(vm,
                                           VIR_DOMAIN_EVENT_STOPPED,
                                           VIR_DOMAIN_EVENT_STOPPED_SHUTDOWN);
        switch ((virDomainLifecycleAction) vm->def->onPoweroff) {
        case VIR_DOMAIN_LIFECYCLE_ACTION_DESTROY:
            libxlDomainShutdownHandleDestroy(driver, vm);
            goto endjob;
        case VIR_DOMAIN_LIFECYCLE_ACTION_RESTART:
        case VIR_DOMAIN_LIFECYCLE_ACTION_RESTART_RENAME:
            libxlDomainShutdownHandleRestart(driver, vm);
            goto endjob;
        case VIR_DOMAIN_LIFECYCLE_ACTION_PRESERVE:
        case VIR_DOMAIN_LIFECYCLE_ACTION_COREDUMP_DESTROY:
        case VIR_DOMAIN_LIFECYCLE_ACTION_COREDUMP_RESTART:
        case VIR_DOMAIN_LIFECYCLE_ACTION_LAST:
            goto endjob;
        }
    } else if (xl_reason == LIBXL_SHUTDOWN_REASON_CRASH) {
        virDomainObjSetState(vm, VIR_DOMAIN_SHUTOFF,
                             VIR_DOMAIN_SHUTOFF_CRASHED);

        dom_event = virDomainEventLifecycleNewFromObj(vm,
                                           VIR_DOMAIN_EVENT_STOPPED,
                                           VIR_DOMAIN_EVENT_STOPPED_CRASHED);
        switch ((virDomainLifecycleAction) vm->def->onCrash) {
        case VIR_DOMAIN_LIFECYCLE_ACTION_DESTROY:
            libxlDomainShutdownHandleDestroy(driver, vm);
            goto endjob;
        case VIR_DOMAIN_LIFECYCLE_ACTION_RESTART:
        case VIR_DOMAIN_LIFECYCLE_ACTION_RESTART_RENAME:
            libxlDomainShutdownHandleRestart(driver, vm);
            goto endjob;
        case VIR_DOMAIN_LIFECYCLE_ACTION_PRESERVE:
        case VIR_DOMAIN_LIFECYCLE_ACTION_LAST:
            goto endjob;
        case VIR_DOMAIN_LIFECYCLE_ACTION_COREDUMP_DESTROY:
            libxlDomainAutoCoreDump(driver, vm);
            libxlDomainShutdownHandleDestroy(driver, vm);
            goto endjob;
        case VIR_DOMAIN_LIFECYCLE_ACTION_COREDUMP_RESTART:
            libxlDomainAutoCoreDump(driver, vm);
            libxlDomainShutdownHandleRestart(driver, vm);
            goto endjob;
        }
    } else if (xl_reason == LIBXL_SHUTDOWN_REASON_REBOOT) {
        virDomainObjSetState(vm, VIR_DOMAIN_SHUTOFF,
                             VIR_DOMAIN_SHUTOFF_SHUTDOWN);

        dom_event = virDomainEventLifecycleNewFromObj(vm,
                                           VIR_DOMAIN_EVENT_STOPPED,
                                           VIR_DOMAIN_EVENT_STOPPED_SHUTDOWN);
        switch ((virDomainLifecycleAction) vm->def->onReboot) {
        case VIR_DOMAIN_LIFECYCLE_ACTION_DESTROY:
            libxlDomainShutdownHandleDestroy(driver, vm);
            goto endjob;
        case VIR_DOMAIN_LIFECYCLE_ACTION_RESTART:
        case VIR_DOMAIN_LIFECYCLE_ACTION_RESTART_RENAME:
            libxlDomainShutdownHandleRestart(driver, vm);
            goto endjob;
        case VIR_DOMAIN_LIFECYCLE_ACTION_PRESERVE:
        case VIR_DOMAIN_LIFECYCLE_ACTION_COREDUMP_DESTROY:
        case VIR_DOMAIN_LIFECYCLE_ACTION_COREDUMP_RESTART:
        case VIR_DOMAIN_LIFECYCLE_ACTION_LAST:
            goto endjob;
        }
    } else if (xl_reason == LIBXL_SHUTDOWN_REASON_SOFT_RESET) {
        libxlDomainObjPrivate *priv = vm->privateData;

        if (libxlRetrieveDomainConfigurationWrapper(cfg->ctx, vm->def->id,
                                                    &d_config) != 0) {
            VIR_ERROR(_("Failed to retrieve config for VM '%s'. "
                        "Unable to perform soft reset. Destroying VM"),
                      vm->def->name);
            libxlDomainShutdownHandleDestroy(driver, vm);
            goto endjob;
        }

        if (priv->deathW) {
            libxl_evdisable_domain_death(cfg->ctx, priv->deathW);
            priv->deathW = NULL;
        }

        if (libxl_domain_soft_reset(cfg->ctx, &d_config, vm->def->id,
                                    NULL, NULL) != 0) {
            VIR_ERROR(_("Failed to soft reset VM '%s'. Destroying VM"),
                      vm->def->name);
            libxlDomainShutdownHandleDestroy(driver, vm);
            goto endjob;
        }
        libxl_evenable_domain_death(cfg->ctx, vm->def->id, 0, &priv->deathW);
        libxlDomainUnpauseWrapper(cfg->ctx, vm->def->id);
    } else {
        VIR_INFO("Unhandled shutdown_reason %d", xl_reason);
    }

 endjob:
    libxlDomainObjEndJob(driver, vm);

 cleanup:
    virDomainObjEndAPI(&vm);
    virObjectEventStateQueue(driver->domainEventState, dom_event);
    libxl_event_free(cfg->ctx, ev);
    VIR_FREE(shutdown_info);
    libxl_domain_config_dispose(&d_config);
}