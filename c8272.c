psutil_proc_connections(PyObject *self, PyObject *args) {
    long pid;
    int pidinfo_result;
    int iterations;
    int i;
    unsigned long nb;

    struct proc_fdinfo *fds_pointer = NULL;
    struct proc_fdinfo *fdp_pointer;
    struct socket_fdinfo si;

    PyObject *py_retlist = PyList_New(0);
    PyObject *py_tuple = NULL;
    PyObject *py_laddr = NULL;
    PyObject *py_raddr = NULL;
    PyObject *py_af_filter = NULL;
    PyObject *py_type_filter = NULL;

    if (py_retlist == NULL)
        return NULL;

    if (! PyArg_ParseTuple(args, "lOO", &pid, &py_af_filter, &py_type_filter))
        goto error;

    if (!PySequence_Check(py_af_filter) || !PySequence_Check(py_type_filter)) {
        PyErr_SetString(PyExc_TypeError, "arg 2 or 3 is not a sequence");
        goto error;
    }

    if (pid == 0)
        return py_retlist;
    pidinfo_result = psutil_proc_pidinfo(pid, PROC_PIDLISTFDS, 0, NULL, 0);
    if (pidinfo_result <= 0)
        goto error;

    fds_pointer = malloc(pidinfo_result);
    if (fds_pointer == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    pidinfo_result = psutil_proc_pidinfo(
        pid, PROC_PIDLISTFDS, 0, fds_pointer, pidinfo_result);
    if (pidinfo_result <= 0)
        goto error;

    iterations = (pidinfo_result / PROC_PIDLISTFD_SIZE);
    for (i = 0; i < iterations; i++) {
        py_tuple = NULL;
        py_laddr = NULL;
        py_raddr = NULL;
        fdp_pointer = &fds_pointer[i];

        if (fdp_pointer->proc_fdtype == PROX_FDTYPE_SOCKET) {
            errno = 0;
            nb = proc_pidfdinfo((pid_t)pid, fdp_pointer->proc_fd,
                                PROC_PIDFDSOCKETINFO, &si, sizeof(si));

            // --- errors checking
            if ((nb <= 0) || (nb < sizeof(si))) {
                if (errno == EBADF) {
                    // let's assume socket has been closed
                    continue;
                }
                else {
                    psutil_raise_for_pid(
                        pid, "proc_pidinfo(PROC_PIDFDSOCKETINFO)");
                    goto error;
                }
            }
            // --- /errors checking

            //
            int fd, family, type, lport, rport, state;
            char lip[200], rip[200];
            int inseq;
            PyObject *py_family;
            PyObject *py_type;

            fd = (int)fdp_pointer->proc_fd;
            family = si.psi.soi_family;
            type = si.psi.soi_type;

            // apply filters
            py_family = PyLong_FromLong((long)family);
            inseq = PySequence_Contains(py_af_filter, py_family);
            Py_DECREF(py_family);
            if (inseq == 0)
                continue;
            py_type = PyLong_FromLong((long)type);
            inseq = PySequence_Contains(py_type_filter, py_type);
            Py_DECREF(py_type);
            if (inseq == 0)
                continue;

            if (errno != 0) {
                PyErr_SetFromErrno(PyExc_OSError);
                goto error;
            }

            if ((family == AF_INET) || (family == AF_INET6)) {
                if (family == AF_INET) {
                    inet_ntop(AF_INET,
                              &si.psi.soi_proto.pri_tcp.tcpsi_ini. \
                                  insi_laddr.ina_46.i46a_addr4,
                              lip,
                              sizeof(lip));
                    inet_ntop(AF_INET,
                              &si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_faddr. \
                                  ina_46.i46a_addr4,
                              rip,
                              sizeof(rip));
                }
                else {
                    inet_ntop(AF_INET6,
                              &si.psi.soi_proto.pri_tcp.tcpsi_ini. \
                                  insi_laddr.ina_6,
                              lip, sizeof(lip));
                    inet_ntop(AF_INET6,
                              &si.psi.soi_proto.pri_tcp.tcpsi_ini. \
                                  insi_faddr.ina_6,
                              rip, sizeof(rip));
                }

                // check for inet_ntop failures
                if (errno != 0) {
                    PyErr_SetFromOSErrnoWithSyscall("inet_ntop()");
                    goto error;
                }

                lport = ntohs(si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_lport);
                rport = ntohs(si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_fport);
                if (type == SOCK_STREAM)
                    state = (int)si.psi.soi_proto.pri_tcp.tcpsi_state;
                else
                    state = PSUTIL_CONN_NONE;

                py_laddr = Py_BuildValue("(si)", lip, lport);
                if (!py_laddr)
                    goto error;
                if (rport != 0)
                    py_raddr = Py_BuildValue("(si)", rip, rport);
                else
                    py_raddr = Py_BuildValue("()");
                if (!py_raddr)
                    goto error;

                // construct the python list
                py_tuple = Py_BuildValue(
                    "(iiiNNi)", fd, family, type, py_laddr, py_raddr, state);
                if (!py_tuple)
                    goto error;
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
            }
            else if (family == AF_UNIX) {
                py_laddr = PyUnicode_DecodeFSDefault(
                    si.psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_path);
                if (!py_laddr)
                    goto error;
                py_raddr = PyUnicode_DecodeFSDefault(
                    si.psi.soi_proto.pri_un.unsi_caddr.ua_sun.sun_path);
                if (!py_raddr)
                    goto error;
                // construct the python list
                py_tuple = Py_BuildValue(
                    "(iiiOOi)",
                    fd, family, type,
                    py_laddr,
                    py_raddr,
                    PSUTIL_CONN_NONE);
                if (!py_tuple)
                    goto error;
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
                Py_DECREF(py_laddr);
                Py_DECREF(py_raddr);
            }
        }
    }

    free(fds_pointer);
    return py_retlist;

error:
    Py_XDECREF(py_tuple);
    Py_XDECREF(py_laddr);
    Py_XDECREF(py_raddr);
    Py_DECREF(py_retlist);
    if (fds_pointer != NULL)
        free(fds_pointer);
    return NULL;
}