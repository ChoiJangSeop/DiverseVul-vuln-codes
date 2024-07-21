storagePoolLookupByTargetPath(virConnectPtr conn,
                              const char *path)
{
    virStoragePoolObj *obj;
    virStoragePoolDef *def;
    virStoragePoolPtr pool = NULL;
    g_autofree char *cleanpath = NULL;

    cleanpath = virFileSanitizePath(path);
    if (!cleanpath)
        return NULL;

    if ((obj = virStoragePoolObjListSearch(driver->pools,
                                           storagePoolLookupByTargetPathCallback,
                                           cleanpath))) {
        def = virStoragePoolObjGetDef(obj);
        if (virStoragePoolLookupByTargetPathEnsureACL(conn, def) < 0)
            return NULL;

        pool = virGetStoragePool(conn, def->name, def->uuid, NULL, NULL);
        virStoragePoolObjEndAPI(&obj);
    }

    if (!pool) {
        if (STREQ(path, cleanpath)) {
            virReportError(VIR_ERR_NO_STORAGE_POOL,
                           _("no storage pool with matching target path '%s'"),
                           path);
        } else {
            virReportError(VIR_ERR_NO_STORAGE_POOL,
                           _("no storage pool with matching target path '%s' (%s)"),
                           path, cleanpath);
        }
    }

    return pool;
}