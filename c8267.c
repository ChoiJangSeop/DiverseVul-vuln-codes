psutil_per_cpu_times(PyObject *self, PyObject *args) {
    kstat_ctl_t *kc;
    kstat_t *ksp;
    cpu_stat_t cs;
    PyObject *py_retlist = PyList_New(0);
    PyObject *py_cputime = NULL;

    if (py_retlist == NULL)
        return NULL;

    kc = kstat_open();
    if (kc == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next) {
        if (strcmp(ksp->ks_module, "cpu_stat") == 0) {
            if (kstat_read(kc, ksp, &cs) == -1) {
                PyErr_SetFromErrno(PyExc_OSError);
                goto error;
            }
            py_cputime = Py_BuildValue("ffff",
                                       (float)cs.cpu_sysinfo.cpu[CPU_USER],
                                       (float)cs.cpu_sysinfo.cpu[CPU_KERNEL],
                                       (float)cs.cpu_sysinfo.cpu[CPU_IDLE],
                                       (float)cs.cpu_sysinfo.cpu[CPU_WAIT]);
            if (py_cputime == NULL)
                goto error;
            if (PyList_Append(py_retlist, py_cputime))
                goto error;
            Py_DECREF(py_cputime);
            py_cputime = NULL;
        }
    }

    kstat_close(kc);
    return py_retlist;

error:
    Py_XDECREF(py_cputime);
    Py_DECREF(py_retlist);
    if (kc != NULL)
        kstat_close(kc);
    return NULL;
}