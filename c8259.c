psutil_net_connections(PyObject *self, PyObject *args) {
    static long null_address[4] = { 0, 0, 0, 0 };
    unsigned long pid;
    int pid_return;
    PVOID table = NULL;
    DWORD tableSize;
    DWORD error;
    PMIB_TCPTABLE_OWNER_PID tcp4Table;
    PMIB_UDPTABLE_OWNER_PID udp4Table;
    PMIB_TCP6TABLE_OWNER_PID tcp6Table;
    PMIB_UDP6TABLE_OWNER_PID udp6Table;
    ULONG i;
    CHAR addressBufferLocal[65];
    CHAR addressBufferRemote[65];

    PyObject *py_retlist;
    PyObject *py_conn_tuple = NULL;
    PyObject *py_af_filter = NULL;
    PyObject *py_type_filter = NULL;
    PyObject *py_addr_tuple_local = NULL;
    PyObject *py_addr_tuple_remote = NULL;
    PyObject *_AF_INET = PyLong_FromLong((long)AF_INET);
    PyObject *_AF_INET6 = PyLong_FromLong((long)AF_INET6);
    PyObject *_SOCK_STREAM = PyLong_FromLong((long)SOCK_STREAM);
    PyObject *_SOCK_DGRAM = PyLong_FromLong((long)SOCK_DGRAM);

    // Import some functions.
    if (! PyArg_ParseTuple(args, "lOO", &pid, &py_af_filter, &py_type_filter))
        goto error;

    if (!PySequence_Check(py_af_filter) || !PySequence_Check(py_type_filter)) {
        psutil_conn_decref_objs();
        PyErr_SetString(PyExc_TypeError, "arg 2 or 3 is not a sequence");
        return NULL;
    }

    if (pid != -1) {
        pid_return = psutil_pid_is_running(pid);
        if (pid_return == 0) {
            psutil_conn_decref_objs();
            return NoSuchProcess("");
        }
        else if (pid_return == -1) {
            psutil_conn_decref_objs();
            return NULL;
        }
    }

    py_retlist = PyList_New(0);
    if (py_retlist == NULL) {
        psutil_conn_decref_objs();
        return NULL;
    }

    // TCP IPv4

    if ((PySequence_Contains(py_af_filter, _AF_INET) == 1) &&
            (PySequence_Contains(py_type_filter, _SOCK_STREAM) == 1))
    {
        table = NULL;
        py_conn_tuple = NULL;
        py_addr_tuple_local = NULL;
        py_addr_tuple_remote = NULL;
        tableSize = 0;

        error = __GetExtendedTcpTable(psutil_GetExtendedTcpTable,
                                      AF_INET, &table, &tableSize);
        if (error != 0)
            goto error;
        tcp4Table = table;
        for (i = 0; i < tcp4Table->dwNumEntries; i++) {
            if (pid != -1) {
                if (tcp4Table->table[i].dwOwningPid != pid) {
                    continue;
                }
            }

            if (tcp4Table->table[i].dwLocalAddr != 0 ||
                    tcp4Table->table[i].dwLocalPort != 0)
            {
                struct in_addr addr;

                addr.S_un.S_addr = tcp4Table->table[i].dwLocalAddr;
                psutil_rtlIpv4AddressToStringA(&addr, addressBufferLocal);
                py_addr_tuple_local = Py_BuildValue(
                    "(si)",
                    addressBufferLocal,
                    BYTESWAP_USHORT(tcp4Table->table[i].dwLocalPort));
            }
            else {
                py_addr_tuple_local = PyTuple_New(0);
            }

            if (py_addr_tuple_local == NULL)
                goto error;

            // On Windows <= XP, remote addr is filled even if socket
            // is in LISTEN mode in which case we just ignore it.
            if ((tcp4Table->table[i].dwRemoteAddr != 0 ||
                    tcp4Table->table[i].dwRemotePort != 0) &&
                    (tcp4Table->table[i].dwState != MIB_TCP_STATE_LISTEN))
            {
                struct in_addr addr;

                addr.S_un.S_addr = tcp4Table->table[i].dwRemoteAddr;
                psutil_rtlIpv4AddressToStringA(&addr, addressBufferRemote);
                py_addr_tuple_remote = Py_BuildValue(
                    "(si)",
                    addressBufferRemote,
                    BYTESWAP_USHORT(tcp4Table->table[i].dwRemotePort));
            }
            else
            {
                py_addr_tuple_remote = PyTuple_New(0);
            }

            if (py_addr_tuple_remote == NULL)
                goto error;

            py_conn_tuple = Py_BuildValue(
                "(iiiNNiI)",
                -1,
                AF_INET,
                SOCK_STREAM,
                py_addr_tuple_local,
                py_addr_tuple_remote,
                tcp4Table->table[i].dwState,
                tcp4Table->table[i].dwOwningPid);
            if (!py_conn_tuple)
                goto error;
            if (PyList_Append(py_retlist, py_conn_tuple))
                goto error;
            Py_DECREF(py_conn_tuple);
        }

        free(table);
        table = NULL;
        tableSize = 0;
    }

    // TCP IPv6
    if ((PySequence_Contains(py_af_filter, _AF_INET6) == 1) &&
            (PySequence_Contains(py_type_filter, _SOCK_STREAM) == 1) &&
            (psutil_rtlIpv6AddressToStringA != NULL))
    {
        table = NULL;
        py_conn_tuple = NULL;
        py_addr_tuple_local = NULL;
        py_addr_tuple_remote = NULL;
        tableSize = 0;

        error = __GetExtendedTcpTable(psutil_GetExtendedTcpTable,
                                      AF_INET6, &table, &tableSize);
        if (error != 0)
            goto error;
        tcp6Table = table;
        for (i = 0; i < tcp6Table->dwNumEntries; i++)
        {
            if (pid != -1) {
                if (tcp6Table->table[i].dwOwningPid != pid) {
                    continue;
                }
            }

            if (memcmp(tcp6Table->table[i].ucLocalAddr, null_address, 16)
                    != 0 || tcp6Table->table[i].dwLocalPort != 0)
            {
                struct in6_addr addr;

                memcpy(&addr, tcp6Table->table[i].ucLocalAddr, 16);
                psutil_rtlIpv6AddressToStringA(&addr, addressBufferLocal);
                py_addr_tuple_local = Py_BuildValue(
                    "(si)",
                    addressBufferLocal,
                    BYTESWAP_USHORT(tcp6Table->table[i].dwLocalPort));
            }
            else {
                py_addr_tuple_local = PyTuple_New(0);
            }

            if (py_addr_tuple_local == NULL)
                goto error;

            // On Windows <= XP, remote addr is filled even if socket
            // is in LISTEN mode in which case we just ignore it.
            if ((memcmp(tcp6Table->table[i].ucRemoteAddr, null_address, 16)
                    != 0 ||
                    tcp6Table->table[i].dwRemotePort != 0) &&
                    (tcp6Table->table[i].dwState != MIB_TCP_STATE_LISTEN))
            {
                struct in6_addr addr;

                memcpy(&addr, tcp6Table->table[i].ucRemoteAddr, 16);
                psutil_rtlIpv6AddressToStringA(&addr, addressBufferRemote);
                py_addr_tuple_remote = Py_BuildValue(
                    "(si)",
                    addressBufferRemote,
                    BYTESWAP_USHORT(tcp6Table->table[i].dwRemotePort));
            }
            else {
                py_addr_tuple_remote = PyTuple_New(0);
            }

            if (py_addr_tuple_remote == NULL)
                goto error;

            py_conn_tuple = Py_BuildValue(
                "(iiiNNiI)",
                -1,
                AF_INET6,
                SOCK_STREAM,
                py_addr_tuple_local,
                py_addr_tuple_remote,
                tcp6Table->table[i].dwState,
                tcp6Table->table[i].dwOwningPid);
            if (!py_conn_tuple)
                goto error;
            if (PyList_Append(py_retlist, py_conn_tuple))
                goto error;
            Py_DECREF(py_conn_tuple);
        }

        free(table);
        table = NULL;
        tableSize = 0;
    }

    // UDP IPv4

    if ((PySequence_Contains(py_af_filter, _AF_INET) == 1) &&
            (PySequence_Contains(py_type_filter, _SOCK_DGRAM) == 1))
    {
        table = NULL;
        py_conn_tuple = NULL;
        py_addr_tuple_local = NULL;
        py_addr_tuple_remote = NULL;
        tableSize = 0;
        error = __GetExtendedUdpTable(psutil_GetExtendedUdpTable,
                                      AF_INET, &table, &tableSize);
        if (error != 0)
            goto error;
        udp4Table = table;
        for (i = 0; i < udp4Table->dwNumEntries; i++)
        {
            if (pid != -1) {
                if (udp4Table->table[i].dwOwningPid != pid) {
                    continue;
                }
            }

            if (udp4Table->table[i].dwLocalAddr != 0 ||
                udp4Table->table[i].dwLocalPort != 0)
            {
                struct in_addr addr;

                addr.S_un.S_addr = udp4Table->table[i].dwLocalAddr;
                psutil_rtlIpv4AddressToStringA(&addr, addressBufferLocal);
                py_addr_tuple_local = Py_BuildValue(
                    "(si)",
                    addressBufferLocal,
                    BYTESWAP_USHORT(udp4Table->table[i].dwLocalPort));
            }
            else {
                py_addr_tuple_local = PyTuple_New(0);
            }

            if (py_addr_tuple_local == NULL)
                goto error;

            py_conn_tuple = Py_BuildValue(
                "(iiiNNiI)",
                -1,
                AF_INET,
                SOCK_DGRAM,
                py_addr_tuple_local,
                PyTuple_New(0),
                PSUTIL_CONN_NONE,
                udp4Table->table[i].dwOwningPid);
            if (!py_conn_tuple)
                goto error;
            if (PyList_Append(py_retlist, py_conn_tuple))
                goto error;
            Py_DECREF(py_conn_tuple);
        }

        free(table);
        table = NULL;
        tableSize = 0;
    }

    // UDP IPv6

    if ((PySequence_Contains(py_af_filter, _AF_INET6) == 1) &&
            (PySequence_Contains(py_type_filter, _SOCK_DGRAM) == 1) &&
            (psutil_rtlIpv6AddressToStringA != NULL))
    {
        table = NULL;
        py_conn_tuple = NULL;
        py_addr_tuple_local = NULL;
        py_addr_tuple_remote = NULL;
        tableSize = 0;
        error = __GetExtendedUdpTable(psutil_GetExtendedUdpTable,
                                      AF_INET6, &table, &tableSize);
        if (error != 0)
            goto error;
        udp6Table = table;
        for (i = 0; i < udp6Table->dwNumEntries; i++) {
            if (pid != -1) {
                if (udp6Table->table[i].dwOwningPid != pid) {
                    continue;
                }
            }

            if (memcmp(udp6Table->table[i].ucLocalAddr, null_address, 16)
                    != 0 || udp6Table->table[i].dwLocalPort != 0)
            {
                struct in6_addr addr;

                memcpy(&addr, udp6Table->table[i].ucLocalAddr, 16);
                psutil_rtlIpv6AddressToStringA(&addr, addressBufferLocal);
                py_addr_tuple_local = Py_BuildValue(
                    "(si)",
                    addressBufferLocal,
                    BYTESWAP_USHORT(udp6Table->table[i].dwLocalPort));
            }
            else {
                py_addr_tuple_local = PyTuple_New(0);
            }

            if (py_addr_tuple_local == NULL)
                goto error;

            py_conn_tuple = Py_BuildValue(
                "(iiiNNiI)",
                -1,
                AF_INET6,
                SOCK_DGRAM,
                py_addr_tuple_local,
                PyTuple_New(0),
                PSUTIL_CONN_NONE,
                udp6Table->table[i].dwOwningPid);
            if (!py_conn_tuple)
                goto error;
            if (PyList_Append(py_retlist, py_conn_tuple))
                goto error;
            Py_DECREF(py_conn_tuple);
        }

        free(table);
        table = NULL;
        tableSize = 0;
    }

    psutil_conn_decref_objs();
    return py_retlist;

error:
    psutil_conn_decref_objs();
    Py_XDECREF(py_conn_tuple);
    Py_XDECREF(py_addr_tuple_local);
    Py_XDECREF(py_addr_tuple_remote);
    Py_DECREF(py_retlist);
    if (table != NULL)
        free(table);
    return NULL;
}