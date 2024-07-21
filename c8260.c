psutil_disk_partitions(PyObject *self, PyObject *args) {
    FILE *file;
    struct mnttab mt;
    PyObject *py_dev = NULL;
    PyObject *py_mountp = NULL;
    PyObject *py_tuple = NULL;
    PyObject *py_retlist = PyList_New(0);

    if (py_retlist == NULL)
        return NULL;

    file = fopen(MNTTAB, "rb");
    if (file == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    while (getmntent(file, &mt) == 0) {
        py_dev = PyUnicode_DecodeFSDefault(mt.mnt_special);
        if (! py_dev)
            goto error;
        py_mountp = PyUnicode_DecodeFSDefault(mt.mnt_mountp);
        if (! py_mountp)
            goto error;
        py_tuple = Py_BuildValue(
            "(OOss)",
            py_dev,           // device
            py_mountp,        // mount point
            mt.mnt_fstype,    // fs type
            mt.mnt_mntopts);  // options
        if (py_tuple == NULL)
            goto error;
        if (PyList_Append(py_retlist, py_tuple))
            goto error;
        Py_DECREF(py_dev);
        Py_DECREF(py_mountp);
        Py_DECREF(py_tuple);
    }
    fclose(file);
    return py_retlist;

error:
    Py_XDECREF(py_dev);
    Py_XDECREF(py_mountp);
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retlist);
    if (file != NULL)
        fclose(file);
    return NULL;
}