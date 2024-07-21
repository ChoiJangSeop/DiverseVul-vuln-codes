psutil_disk_io_counters(PyObject *self, PyObject *args) {
    kstat_ctl_t *kc;
    kstat_t *ksp;
    kstat_io_t kio;
    PyObject *py_retdict = PyDict_New();
    PyObject *py_disk_info = NULL;

    if (py_retdict == NULL)
        return NULL;
    kc = kstat_open();
    if (kc == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);;
        goto error;
    }
    ksp = kc->kc_chain;
    while (ksp != NULL) {
        if (ksp->ks_type == KSTAT_TYPE_IO) {
            if (strcmp(ksp->ks_class, "disk") == 0) {
                if (kstat_read(kc, ksp, &kio) == -1) {
                    kstat_close(kc);
                    return PyErr_SetFromErrno(PyExc_OSError);;
                }
                py_disk_info = Py_BuildValue(
                    "(IIKKLL)",
                    kio.reads,
                    kio.writes,
                    kio.nread,
                    kio.nwritten,
                    kio.rtime / 1000 / 1000,  // from nano to milli secs
                    kio.wtime / 1000 / 1000   // from nano to milli secs
                );
                if (!py_disk_info)
                    goto error;
                if (PyDict_SetItemString(py_retdict, ksp->ks_name,
                                         py_disk_info))
                    goto error;
                Py_DECREF(py_disk_info);
            }
        }
        ksp = ksp->ks_next;
    }
    kstat_close(kc);

    return py_retdict;

error:
    Py_XDECREF(py_disk_info);
    Py_DECREF(py_retdict);
    if (kc != NULL)
        kstat_close(kc);
    return NULL;
}