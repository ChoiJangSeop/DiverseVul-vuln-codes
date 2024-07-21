psutil_users(PyObject *self, PyObject *args) {
    HANDLE hServer = WTS_CURRENT_SERVER_HANDLE;
    WCHAR *buffer_user = NULL;
    LPTSTR buffer_addr = NULL;
    PWTS_SESSION_INFO sessions = NULL;
    DWORD count;
    DWORD i;
    DWORD sessionId;
    DWORD bytes;
    PWTS_CLIENT_ADDRESS address;
    char address_str[50];
    long long unix_time;
    WINSTATION_INFO station_info;
    ULONG returnLen;
    PyObject *py_tuple = NULL;
    PyObject *py_address = NULL;
    PyObject *py_username = NULL;
    PyObject *py_retlist = PyList_New(0);

    if (py_retlist == NULL)
        return NULL;

    if (WTSEnumerateSessions(hServer, 0, 1, &sessions, &count) == 0) {
        PyErr_SetFromOSErrnoWithSyscall("WTSEnumerateSessions");
        goto error;
    }

    for (i = 0; i < count; i++) {
        py_address = NULL;
        py_tuple = NULL;
        sessionId = sessions[i].SessionId;
        if (buffer_user != NULL)
            WTSFreeMemory(buffer_user);
        if (buffer_addr != NULL)
            WTSFreeMemory(buffer_addr);

        buffer_user = NULL;
        buffer_addr = NULL;

        // username
        bytes = 0;
        if (WTSQuerySessionInformationW(hServer, sessionId, WTSUserName,
                                        &buffer_user, &bytes) == 0) {
            PyErr_SetFromOSErrnoWithSyscall("WTSQuerySessionInformationW");
            goto error;
        }
        if (bytes <= 2)
            continue;

        // address
        bytes = 0;
        if (WTSQuerySessionInformation(hServer, sessionId, WTSClientAddress,
                                       &buffer_addr, &bytes) == 0) {
            PyErr_SetFromOSErrnoWithSyscall("WTSQuerySessionInformation");
            goto error;
        }

        address = (PWTS_CLIENT_ADDRESS)buffer_addr;
        if (address->AddressFamily == 0) {  // AF_INET
            sprintf_s(address_str,
                      _countof(address_str),
                      "%u.%u.%u.%u",
                      address->Address[0],
                      address->Address[1],
                      address->Address[2],
                      address->Address[3]);
            py_address = Py_BuildValue("s", address_str);
            if (!py_address)
                goto error;
        }
        else {
            py_address = Py_None;
        }

        // login time
        if (! psutil_WinStationQueryInformationW(
                hServer,
                sessionId,
                WinStationInformation,
                &station_info,
                sizeof(station_info),
                &returnLen))
        {
            PyErr_SetFromOSErrnoWithSyscall("WinStationQueryInformationW");
            goto error;
        }

        unix_time = ((LONGLONG)station_info.ConnectTime.dwHighDateTime) << 32;
        unix_time += \
            station_info.ConnectTime.dwLowDateTime - 116444736000000000LL;
        unix_time /= 10000000;

        py_username = PyUnicode_FromWideChar(buffer_user, wcslen(buffer_user));
        if (py_username == NULL)
            goto error;
        py_tuple = Py_BuildValue("OOd",
                                 py_username,
                                 py_address,
                                 (double)unix_time);
        if (!py_tuple)
            goto error;
        if (PyList_Append(py_retlist, py_tuple))
            goto error;
        Py_XDECREF(py_username);
        Py_XDECREF(py_address);
        Py_XDECREF(py_tuple);
    }

    WTSFreeMemory(sessions);
    WTSFreeMemory(buffer_user);
    WTSFreeMemory(buffer_addr);
    return py_retlist;

error:
    Py_XDECREF(py_username);
    Py_XDECREF(py_tuple);
    Py_XDECREF(py_address);
    Py_DECREF(py_retlist);

    if (sessions != NULL)
        WTSFreeMemory(sessions);
    if (buffer_user != NULL)
        WTSFreeMemory(buffer_user);
    if (buffer_addr != NULL)
        WTSFreeMemory(buffer_addr);
    return NULL;
}