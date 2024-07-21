libxlDomainHandleDeath(libxlDriverPrivate *driver, virDomainObj *vm)
{
    virObjectEvent *dom_event = NULL;

    if (libxlDomainObjBeginJob(driver, vm, LIBXL_JOB_MODIFY) < 0)
        return;

    virDomainObjSetState(vm, VIR_DOMAIN_SHUTOFF, VIR_DOMAIN_SHUTOFF_DESTROYED);
    dom_event = virDomainEventLifecycleNewFromObj(vm,
                                                  VIR_DOMAIN_EVENT_STOPPED,
                                                  VIR_DOMAIN_EVENT_STOPPED_DESTROYED);
    libxlDomainCleanup(driver, vm);
    if (!vm->persistent)
        virDomainObjListRemove(driver->domains, vm);
    libxlDomainObjEndJob(driver, vm);
    virObjectEventStateQueue(driver->domainEventState, dom_event);
}