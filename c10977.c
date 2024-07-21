libxlDomainDestroyInternal(libxlDriverPrivate *driver,
                           virDomainObj *vm)
{
    g_autoptr(libxlDriverConfig) cfg = libxlDriverConfigGet(driver);
    libxlDomainObjPrivate *priv = vm->privateData;
    int ret = -1;

    /* Ignore next LIBXL_EVENT_TYPE_DOMAIN_DEATH as the caller will handle
     * domain death appropriately already (having more info, like the reason).
     */
    priv->ignoreDeathEvent = true;
    /* Unlock virDomainObj during destroy, which can take considerable
     * time on large memory domains.
     */
    virObjectUnlock(vm);
    ret = libxl_domain_destroy(cfg->ctx, vm->def->id, NULL);
    virObjectLock(vm);
    if (ret)
        priv->ignoreDeathEvent = false;

    return ret;
}