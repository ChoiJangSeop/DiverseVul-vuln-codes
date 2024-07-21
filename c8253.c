psutil_proc_threads(PyObject *self, PyObject *args) {
    HANDLE hThread;
    THREADENTRY32 te32 = {0};
    long pid;
    int pid_return;
    int rc;
    FILETIME ftDummy, ftKernel, ftUser;
    HANDLE hThreadSnap = NULL;
    PyObject *py_tuple = NULL;
    PyObject *py_retlist = PyList_New(0);

    if (py_retlist == NULL)
        return NULL;
    if (! PyArg_ParseTuple(args, "l", &pid))
        goto error;
    if (pid == 0) {
        // raise AD instead of returning 0 as procexp is able to
        // retrieve useful information somehow
        AccessDenied("");
        goto error;
    }

    pid_return = psutil_pid_is_running(pid);
    if (pid_return == 0) {
        NoSuchProcess("");
        goto error;
    }
    if (pid_return == -1)
        goto error;

    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE) {
        PyErr_SetFromOSErrnoWithSyscall("CreateToolhelp32Snapshot");
        goto error;
    }

    // Fill in the size of the structure before using it
    te32.dwSize = sizeof(THREADENTRY32);

    if (! Thread32First(hThreadSnap, &te32)) {
        PyErr_SetFromOSErrnoWithSyscall("Thread32First");
        goto error;
    }

    // Walk the thread snapshot to find all threads of the process.
    // If the thread belongs to the process, increase the counter.
    do {
        if (te32.th32OwnerProcessID == pid) {
            py_tuple = NULL;
            hThread = NULL;
            hThread = OpenThread(THREAD_QUERY_INFORMATION,
                                 FALSE, te32.th32ThreadID);
            if (hThread == NULL) {
                // thread has disappeared on us
                continue;
            }

            rc = GetThreadTimes(hThread, &ftDummy, &ftDummy, &ftKernel,
                                &ftUser);
            if (rc == 0) {
                PyErr_SetFromOSErrnoWithSyscall("GetThreadTimes");
                goto error;
            }

            /*
             * User and kernel times are represented as a FILETIME structure
             * which contains a 64-bit value representing the number of
             * 100-nanosecond intervals since January 1, 1601 (UTC):
             * http://msdn.microsoft.com/en-us/library/ms724284(VS.85).aspx
             * To convert it into a float representing the seconds that the
             * process has executed in user/kernel mode I borrowed the code
             * below from Python's Modules/posixmodule.c
             */
            py_tuple = Py_BuildValue(
                "kdd",
                te32.th32ThreadID,
                (double)(ftUser.dwHighDateTime * 429.4967296 + \
                         ftUser.dwLowDateTime * 1e-7),
                (double)(ftKernel.dwHighDateTime * 429.4967296 + \
                         ftKernel.dwLowDateTime * 1e-7));
            if (!py_tuple)
                goto error;
            if (PyList_Append(py_retlist, py_tuple))
                goto error;
            Py_DECREF(py_tuple);

            CloseHandle(hThread);
        }
    } while (Thread32Next(hThreadSnap, &te32));

    CloseHandle(hThreadSnap);
    return py_retlist;

error:
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retlist);
    if (hThread != NULL)
        CloseHandle(hThread);
    if (hThreadSnap != NULL)
        CloseHandle(hThreadSnap);
    return NULL;
}