psutil_disk_partitions(PyObject *self, PyObject *args) {
    FILE *file = NULL;
    struct mntent * mt = NULL;
    PyObject *py_dev = NULL;
    PyObject *py_mountp = NULL;
    PyObject *py_tuple = NULL;
    PyObject *py_retlist = PyList_New(0);

    if (py_retlist == NULL)
        return NULL;

    file = setmntent(MNTTAB, "rb");
    if (file == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }
    mt = getmntent(file);
    while (mt != NULL) {
        py_dev = PyUnicode_DecodeFSDefault(mt->mnt_fsname);
        if (! py_dev)
            goto error;
        py_mountp = PyUnicode_DecodeFSDefault(mt->mnt_dir);
        if (! py_mountp)
            goto error;
        py_tuple = Py_BuildValue(
            "(OOss)",
            py_dev,         // device
            py_mountp,      // mount point
            mt->mnt_type,   // fs type
            mt->mnt_opts);  // options
        if (py_tuple == NULL)
            goto error;
        if (PyList_Append(py_retlist, py_tuple))
            goto error;
        Py_DECREF(py_dev);
        Py_DECREF(py_mountp);
        Py_DECREF(py_tuple);
        mt = getmntent(file);
    }
    endmntent(file);
    return py_retlist;

error:
    Py_XDECREF(py_dev);
    Py_XDECREF(py_mountp);
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retlist);
    if (file != NULL)
        endmntent(file);
    return NULL;
}