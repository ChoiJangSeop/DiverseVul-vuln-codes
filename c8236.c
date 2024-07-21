psutil_proc_memory_maps(PyObject *self, PyObject *args) {
    MEMORY_BASIC_INFORMATION basicInfo;
    DWORD pid;
    HANDLE hProcess = NULL;
    PVOID baseAddress;
    ULONGLONG previousAllocationBase;
    WCHAR mappedFileName[MAX_PATH];
    LPVOID maxAddr;
    // required by GetMappedFileNameW
    DWORD access = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
    PyObject *py_retlist = PyList_New(0);
    PyObject *py_tuple = NULL;
    PyObject *py_str = NULL;

    if (py_retlist == NULL)
        return NULL;
    if (! PyArg_ParseTuple(args, "l", &pid))
        goto error;
    hProcess = psutil_handle_from_pid(pid, access);
    if (NULL == hProcess)
        goto error;

    maxAddr = PSUTIL_SYSTEM_INFO.lpMaximumApplicationAddress;
    baseAddress = NULL;

    while (VirtualQueryEx(hProcess, baseAddress, &basicInfo,
                          sizeof(MEMORY_BASIC_INFORMATION)))
    {
        py_tuple = NULL;
        if (baseAddress > maxAddr)
            break;
        if (GetMappedFileNameW(hProcess, baseAddress, mappedFileName,
                               sizeof(mappedFileName)))
        {
            py_str = PyUnicode_FromWideChar(mappedFileName,
                                            wcslen(mappedFileName));
            if (py_str == NULL)
                goto error;
#ifdef _WIN64
           py_tuple = Py_BuildValue(
              "(KsOI)",
              (unsigned long long)baseAddress,
#else
           py_tuple = Py_BuildValue(
              "(ksOI)",
              (unsigned long)baseAddress,
#endif
              get_region_protection_string(basicInfo.Protect),
              py_str,
              basicInfo.RegionSize);

            if (!py_tuple)
                goto error;
            if (PyList_Append(py_retlist, py_tuple))
                goto error;
            Py_DECREF(py_tuple);
            Py_DECREF(py_str);
        }
        previousAllocationBase = (ULONGLONG)basicInfo.AllocationBase;
        baseAddress = (PCHAR)baseAddress + basicInfo.RegionSize;
    }

    CloseHandle(hProcess);
    return py_retlist;

error:
    Py_XDECREF(py_tuple);
    Py_XDECREF(py_str);
    Py_DECREF(py_retlist);
    if (hProcess != NULL)
        CloseHandle(hProcess);
    return NULL;
}