psutil_net_if_stats(PyObject* self, PyObject* args) {
    kstat_ctl_t *kc = NULL;
    kstat_t *ksp;
    kstat_named_t *knp;
    int ret;
    int sock = -1;
    int duplex;
    int speed;

    PyObject *py_retdict = PyDict_New();
    PyObject *py_ifc_info = NULL;
    PyObject *py_is_up = NULL;

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

    for (ksp = kc->kc_chain; ksp; ksp = ksp->ks_next) {
        if (strcmp(ksp->ks_class, "net") == 0) {
            struct lifreq ifr;

            kstat_read(kc, ksp, NULL);
            if (ksp->ks_type != KSTAT_TYPE_NAMED)
                continue;
            if (strcmp(ksp->ks_class, "net") != 0)
                continue;

            strncpy(ifr.lifr_name, ksp->ks_name, sizeof(ifr.lifr_name));
            ret = ioctl(sock, SIOCGLIFFLAGS, &ifr);
            if (ret == -1)
                continue;  // not a network interface

            // is up?
            if ((ifr.lifr_flags & IFF_UP) != 0) {
                if ((knp = kstat_data_lookup(ksp, "link_up")) != NULL) {
                    if (knp->value.ui32 != 0u)
                        py_is_up = Py_True;
                    else
                        py_is_up = Py_False;
                }
                else {
                    py_is_up = Py_True;
                }
            }
            else {
                py_is_up = Py_False;
            }
            Py_INCREF(py_is_up);

            // duplex
            duplex = 0;  // unknown
            if ((knp = kstat_data_lookup(ksp, "link_duplex")) != NULL) {
                if (knp->value.ui32 == 1)
                    duplex = 1;  // half
                else if (knp->value.ui32 == 2)
                    duplex = 2;  // full
            }

            // speed
            if ((knp = kstat_data_lookup(ksp, "ifspeed")) != NULL)
                // expressed in bits per sec, we want mega bits per sec
                speed = (int)knp->value.ui64 / 1000000;
            else
                speed = 0;

            // mtu
            ret = ioctl(sock, SIOCGLIFMTU, &ifr);
            if (ret == -1)
                goto error;

            py_ifc_info = Py_BuildValue("(Oiii)", py_is_up, duplex, speed,
                                        ifr.lifr_mtu);
            if (!py_ifc_info)
                goto error;
            if (PyDict_SetItemString(py_retdict, ksp->ks_name, py_ifc_info))
                goto error;
            Py_DECREF(py_ifc_info);
        }
    }

    close(sock);
    kstat_close(kc);
    return py_retdict;

error:
    Py_XDECREF(py_is_up);
    Py_XDECREF(py_ifc_info);
    Py_DECREF(py_retdict);
    if (sock != -1)
        close(sock);
    if (kc != NULL)
        kstat_close(kc);
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
}