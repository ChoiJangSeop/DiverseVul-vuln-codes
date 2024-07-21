psutil_net_io_counters(PyObject *self, PyObject *args) {
    kstat_ctl_t    *kc = NULL;
    kstat_t *ksp;
    kstat_named_t *rbytes, *wbytes, *rpkts, *wpkts, *ierrs, *oerrs;
    int ret;
    int sock = -1;
    struct lifreq ifr;

    PyObject *py_retdict = PyDict_New();
    PyObject *py_ifc_info = NULL;

    if (py_retdict == NULL)
        return NULL;
    kc = kstat_open();
    if (kc == NULL)
        goto error;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    ksp = kc->kc_chain;
    while (ksp != NULL) {
        if (ksp->ks_type != KSTAT_TYPE_NAMED)
            goto next;
        if (strcmp(ksp->ks_class, "net") != 0)
            goto next;
        // skip 'lo' (localhost) because it doesn't have the statistics we need
        // and it makes kstat_data_lookup() fail
        if (strcmp(ksp->ks_module, "lo") == 0)
            goto next;

        // check if this is a network interface by sending a ioctl
        strncpy(ifr.lifr_name, ksp->ks_name, sizeof(ifr.lifr_name));
        ret = ioctl(sock, SIOCGLIFFLAGS, &ifr);
        if (ret == -1)
            goto next;

        if (kstat_read(kc, ksp, NULL) == -1) {
            errno = 0;
            goto next;
        }

        rbytes = (kstat_named_t *)kstat_data_lookup(ksp, "rbytes");
        wbytes = (kstat_named_t *)kstat_data_lookup(ksp, "obytes");
        rpkts = (kstat_named_t *)kstat_data_lookup(ksp, "ipackets");
        wpkts = (kstat_named_t *)kstat_data_lookup(ksp, "opackets");
        ierrs = (kstat_named_t *)kstat_data_lookup(ksp, "ierrors");
        oerrs = (kstat_named_t *)kstat_data_lookup(ksp, "oerrors");

        if ((rbytes == NULL) || (wbytes == NULL) || (rpkts == NULL) ||
                (wpkts == NULL) || (ierrs == NULL) || (oerrs == NULL))
        {
            PyErr_SetString(PyExc_RuntimeError, "kstat_data_lookup() failed");
            goto error;
        }

        if (rbytes->data_type == KSTAT_DATA_UINT64)
        {
            py_ifc_info = Py_BuildValue("(KKKKIIii)",
                                        wbytes->value.ui64,
                                        rbytes->value.ui64,
                                        wpkts->value.ui64,
                                        rpkts->value.ui64,
                                        ierrs->value.ui32,
                                        oerrs->value.ui32,
                                        0,  // dropin not supported
                                        0   // dropout not supported
                                       );
        }
        else
        {
            py_ifc_info = Py_BuildValue("(IIIIIIii)",
                                        wbytes->value.ui32,
                                        rbytes->value.ui32,
                                        wpkts->value.ui32,
                                        rpkts->value.ui32,
                                        ierrs->value.ui32,
                                        oerrs->value.ui32,
                                        0,  // dropin not supported
                                        0   // dropout not supported
                                       );
        }
        if (!py_ifc_info)
            goto error;
        if (PyDict_SetItemString(py_retdict, ksp->ks_name, py_ifc_info))
            goto error;
        Py_DECREF(py_ifc_info);
        goto next;

next:
        ksp = ksp->ks_next;
    }

    kstat_close(kc);
    close(sock);
    return py_retdict;

error:
    Py_XDECREF(py_ifc_info);
    Py_DECREF(py_retdict);
    if (kc != NULL)
        kstat_close(kc);
    if (sock != -1) {
        close(sock);
    }
    return NULL;
}