psutil_pids(PyObject *self, PyObject *args) {
    kinfo_proc *proclist = NULL;
    kinfo_proc *orig_address = NULL;
    size_t num_processes;
    size_t idx;
    PyObject *py_retlist = PyList_New(0);
    PyObject *py_pid = NULL;

    if (py_retlist == NULL)
        return NULL;

    // TODO: RuntimeError is inappropriate here; we could return the
    // original error instead.
    if (psutil_get_proc_list(&proclist, &num_processes) != 0) {
        if (errno != 0) {
            PyErr_SetFromErrno(PyExc_OSError);
        }
        else {
            PyErr_SetString(PyExc_RuntimeError,
                            "failed to retrieve process list");
        }
        goto error;
    }

    if (num_processes > 0) {
        orig_address = proclist; // save so we can free it after we're done
        for (idx = 0; idx < num_processes; idx++) {
#ifdef PSUTIL_FREEBSD
            py_pid = Py_BuildValue("i", proclist->ki_pid);
#elif defined(PSUTIL_OPENBSD) || defined(PSUTIL_NETBSD)
            py_pid = Py_BuildValue("i", proclist->p_pid);
#endif
            if (!py_pid)
                goto error;
            if (PyList_Append(py_retlist, py_pid))
                goto error;
            Py_DECREF(py_pid);
            proclist++;
        }
        free(orig_address);
    }

    return py_retlist;

error:
    Py_XDECREF(py_pid);
    Py_DECREF(py_retlist);
    if (orig_address != NULL)
        free(orig_address);
    return NULL;
}