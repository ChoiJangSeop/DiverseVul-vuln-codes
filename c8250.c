psutil_pids(PyObject *self, PyObject *args) {
    DWORD *proclist = NULL;
    DWORD numberOfReturnedPIDs;
    DWORD i;
    PyObject *py_pid = NULL;
    PyObject *py_retlist = PyList_New(0);

    if (py_retlist == NULL)
        return NULL;
    proclist = psutil_get_pids(&numberOfReturnedPIDs);
    if (proclist == NULL)
        goto error;

    for (i = 0; i < numberOfReturnedPIDs; i++) {
        py_pid = Py_BuildValue("I", proclist[i]);
        if (!py_pid)
            goto error;
        if (PyList_Append(py_retlist, py_pid))
            goto error;
        Py_DECREF(py_pid);
    }

    // free C array allocated for PIDs
    free(proclist);
    return py_retlist;

error:
    Py_XDECREF(py_pid);
    Py_DECREF(py_retlist);
    if (proclist != NULL)
        free(proclist);
    return NULL;
}