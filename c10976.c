libxlDomainCleanup(libxlDriverPrivate *driver,
                   virDomainObj *vm)
{
    libxlDomainObjPrivate *priv = vm->privateData;
    g_autoptr(libxlDriverConfig) cfg = libxlDriverConfigGet(driver);
    int vnc_port;
    char *file;
    virHostdevManager *hostdev_mgr = driver->hostdevMgr;
    unsigned int hostdev_flags = VIR_HOSTDEV_SP_PCI;

    VIR_DEBUG("Cleaning up domain with id '%d' and name '%s'",
              vm->def->id, vm->def->name);

    hostdev_flags |= VIR_HOSTDEV_SP_USB;

    /* Call hook with stopped operation. Ignore error and continue with cleanup */
    ignore_value(libxlDomainHookRun(driver, vm->def, 0,
                                    VIR_HOOK_LIBXL_OP_STOPPED,
                                    VIR_HOOK_SUBOP_END, NULL));

    virHostdevReAttachDomainDevices(hostdev_mgr, LIBXL_DRIVER_INTERNAL_NAME,
                                    vm->def, hostdev_flags);

    if (priv->lockProcessRunning) {
        VIR_FREE(priv->lockState);
        if (virDomainLockProcessPause(driver->lockManager, vm, &priv->lockState) < 0)
            VIR_WARN("Unable to release lease on %s", vm->def->name);
        else
            priv->lockProcessRunning = false;
        VIR_DEBUG("Preserving lock state '%s'", NULLSTR(priv->lockState));
    }

    libxlLoggerCloseFile(cfg->logger, vm->def->id);
    vm->def->id = -1;

    if (priv->deathW) {
        libxl_evdisable_domain_death(cfg->ctx, priv->deathW);
        priv->deathW = NULL;
    }

    priv->ignoreDeathEvent = false;

    if (!!g_atomic_int_dec_and_test(&driver->nactive) && driver->inhibitCallback)
        driver->inhibitCallback(false, driver->inhibitOpaque);

    if ((vm->def->ngraphics == 1) &&
        vm->def->graphics[0]->type == VIR_DOMAIN_GRAPHICS_TYPE_VNC &&
        vm->def->graphics[0]->data.vnc.autoport) {
        vnc_port = vm->def->graphics[0]->data.vnc.port;
        if (vnc_port >= LIBXL_VNC_PORT_MIN) {
            if (virPortAllocatorRelease(vnc_port) < 0)
                VIR_DEBUG("Could not mark port %d as unused", vnc_port);
        }
    }

    libxlNetworkUnwindDevices(vm->def);

    file = g_strdup_printf("%s/%s.xml", cfg->stateDir, vm->def->name);
    if (unlink(file) < 0 && errno != ENOENT && errno != ENOTDIR)
        VIR_DEBUG("Failed to remove domain XML for %s", vm->def->name);
    VIR_FREE(file);

    /* Call hook with release operation. Ignore error and continue with cleanup */
    ignore_value(libxlDomainHookRun(driver, vm->def, 0,
                                    VIR_HOOK_LIBXL_OP_RELEASE,
                                    VIR_HOOK_SUBOP_END, NULL));

    virDomainObjRemoveTransientDef(vm);
}