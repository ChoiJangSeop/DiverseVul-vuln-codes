psutil_disk_partitions(PyObject *self, PyObject *args) {
    int num;
    int i;
    int len;
    uint64_t flags;
    char opts[400];
    struct statfs *fs = NULL;
    PyObject *py_dev = NULL;
    PyObject *py_mountp = NULL;
    PyObject *py_tuple = NULL;
    PyObject *py_retlist = PyList_New(0);

    if (py_retlist == NULL)
        return NULL;

    // get the number of mount points
    Py_BEGIN_ALLOW_THREADS
    num = getfsstat(NULL, 0, MNT_NOWAIT);
    Py_END_ALLOW_THREADS
    if (num == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    len = sizeof(*fs) * num;
    fs = malloc(len);
    if (fs == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    Py_BEGIN_ALLOW_THREADS
    num = getfsstat(fs, len, MNT_NOWAIT);
    Py_END_ALLOW_THREADS
    if (num == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    for (i = 0; i < num; i++) {
        opts[0] = 0;
        flags = fs[i].f_flags;

        // see sys/mount.h
        if (flags & MNT_RDONLY)
            strlcat(opts, "ro", sizeof(opts));
        else
            strlcat(opts, "rw", sizeof(opts));
        if (flags & MNT_SYNCHRONOUS)
            strlcat(opts, ",sync", sizeof(opts));
        if (flags & MNT_NOEXEC)
            strlcat(opts, ",noexec", sizeof(opts));
        if (flags & MNT_NOSUID)
            strlcat(opts, ",nosuid", sizeof(opts));
        if (flags & MNT_UNION)
            strlcat(opts, ",union", sizeof(opts));
        if (flags & MNT_ASYNC)
            strlcat(opts, ",async", sizeof(opts));
        if (flags & MNT_EXPORTED)
            strlcat(opts, ",exported", sizeof(opts));
        if (flags & MNT_QUARANTINE)
            strlcat(opts, ",quarantine", sizeof(opts));
        if (flags & MNT_LOCAL)
            strlcat(opts, ",local", sizeof(opts));
        if (flags & MNT_QUOTA)
            strlcat(opts, ",quota", sizeof(opts));
        if (flags & MNT_ROOTFS)
            strlcat(opts, ",rootfs", sizeof(opts));
        if (flags & MNT_DOVOLFS)
            strlcat(opts, ",dovolfs", sizeof(opts));
        if (flags & MNT_DONTBROWSE)
            strlcat(opts, ",dontbrowse", sizeof(opts));
        if (flags & MNT_IGNORE_OWNERSHIP)
            strlcat(opts, ",ignore-ownership", sizeof(opts));
        if (flags & MNT_AUTOMOUNTED)
            strlcat(opts, ",automounted", sizeof(opts));
        if (flags & MNT_JOURNALED)
            strlcat(opts, ",journaled", sizeof(opts));
        if (flags & MNT_NOUSERXATTR)
            strlcat(opts, ",nouserxattr", sizeof(opts));
        if (flags & MNT_DEFWRITE)
            strlcat(opts, ",defwrite", sizeof(opts));
        if (flags & MNT_MULTILABEL)
            strlcat(opts, ",multilabel", sizeof(opts));
        if (flags & MNT_NOATIME)
            strlcat(opts, ",noatime", sizeof(opts));
        if (flags & MNT_UPDATE)
            strlcat(opts, ",update", sizeof(opts));
        if (flags & MNT_RELOAD)
            strlcat(opts, ",reload", sizeof(opts));
        if (flags & MNT_FORCE)
            strlcat(opts, ",force", sizeof(opts));
        if (flags & MNT_CMDFLAGS)
            strlcat(opts, ",cmdflags", sizeof(opts));

        py_dev = PyUnicode_DecodeFSDefault(fs[i].f_mntfromname);
        if (! py_dev)
            goto error;
        py_mountp = PyUnicode_DecodeFSDefault(fs[i].f_mntonname);
        if (! py_mountp)
            goto error;
        py_tuple = Py_BuildValue(
            "(OOss)",
            py_dev,               // device
            py_mountp,            // mount point
            fs[i].f_fstypename,   // fs type
            opts);                // options
        if (!py_tuple)
            goto error;
        if (PyList_Append(py_retlist, py_tuple))
            goto error;
        Py_DECREF(py_dev);
        Py_DECREF(py_mountp);
        Py_DECREF(py_tuple);
    }

    free(fs);
    return py_retlist;

error:
    Py_XDECREF(py_dev);
    Py_XDECREF(py_mountp);
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retlist);
    if (fs != NULL)
        free(fs);
    return NULL;
}