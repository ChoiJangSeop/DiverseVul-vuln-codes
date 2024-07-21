psutil_ppid_map(PyObject *self, PyObject *args) {
    PyObject *py_pid = NULL;
    PyObject *py_ppid = NULL;
    PyObject *py_retdict = PyDict_New();
    HANDLE handle = NULL;
    PROCESSENTRY32 pe = {0};
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (py_retdict == NULL)
        return NULL;
    handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (handle == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
        Py_DECREF(py_retdict);
        return NULL;
    }

    if (Process32First(handle, &pe)) {
        do {
            py_pid = Py_BuildValue("I", pe.th32ProcessID);
            if (py_pid == NULL)
                goto error;
            py_ppid = Py_BuildValue("I", pe.th32ParentProcessID);
            if (py_ppid == NULL)
                goto error;
            if (PyDict_SetItem(py_retdict, py_pid, py_ppid))
                goto error;
            Py_DECREF(py_pid);
            Py_DECREF(py_ppid);
        } while (Process32Next(handle, &pe));
    }

    CloseHandle(handle);
    return py_retdict;

error:
    Py_XDECREF(py_pid);
    Py_XDECREF(py_ppid);
    Py_DECREF(py_retdict);
    CloseHandle(handle);
    return NULL;
}