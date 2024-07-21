libxlDomainDeathThread(void *opaque)
{
    struct libxlEventHandlerThreadInfo *death_info = opaque;
    virDomainObj *vm = death_info->vm;
    libxl_event *ev = death_info->event;
    libxlDriverPrivate *driver = death_info->driver;
    virObjectEvent *dom_event = NULL;
    g_autoptr(libxlDriverConfig) cfg = libxlDriverConfigGet(driver);

    if (libxlDomainObjBeginJob(driver, vm, LIBXL_JOB_MODIFY) < 0)
        goto cleanup;

    virDomainObjSetState(vm, VIR_DOMAIN_SHUTOFF, VIR_DOMAIN_SHUTOFF_DESTROYED);
    dom_event = virDomainEventLifecycleNewFromObj(vm,
                                                  VIR_DOMAIN_EVENT_STOPPED,
                                                  VIR_DOMAIN_EVENT_STOPPED_DESTROYED);
    libxlDomainCleanup(driver, vm);
    if (!vm->persistent)
        virDomainObjListRemove(driver->domains, vm);
    libxlDomainObjEndJob(driver, vm);
    virObjectEventStateQueue(driver->domainEventState, dom_event);

 cleanup:
    virDomainObjEndAPI(&vm);
    libxl_event_free(cfg->ctx, ev);
    VIR_FREE(death_info);
}