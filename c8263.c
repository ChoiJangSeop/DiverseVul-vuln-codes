psutil_net_io_counters(PyObject *self, PyObject *args) {
    DWORD dwRetVal = 0;

#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
    MIB_IF_ROW2 *pIfRow = NULL;
#else // Windows XP
    MIB_IFROW *pIfRow = NULL;
#endif

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PyObject *py_retdict = PyDict_New();
    PyObject *py_nic_info = NULL;
    PyObject *py_nic_name = NULL;

    if (py_retdict == NULL)
        return NULL;
    pAddresses = psutil_get_nic_addresses();
    if (pAddresses == NULL)
        goto error;
    pCurrAddresses = pAddresses;

    while (pCurrAddresses) {
        py_nic_name = NULL;
        py_nic_info = NULL;

#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
        pIfRow = (MIB_IF_ROW2 *) malloc(sizeof(MIB_IF_ROW2));
#else // Windows XP
        pIfRow = (MIB_IFROW *) malloc(sizeof(MIB_IFROW));
#endif

        if (pIfRow == NULL) {
            PyErr_NoMemory();
            goto error;
        }

#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
        SecureZeroMemory((PVOID)pIfRow, sizeof(MIB_IF_ROW2));
        pIfRow->InterfaceIndex = pCurrAddresses->IfIndex;
        dwRetVal = GetIfEntry2(pIfRow);
#else // Windows XP
        pIfRow->dwIndex = pCurrAddresses->IfIndex;
        dwRetVal = GetIfEntry(pIfRow);
#endif

        if (dwRetVal != NO_ERROR) {
            PyErr_SetString(PyExc_RuntimeError,
                            "GetIfEntry() or GetIfEntry2() syscalls failed.");
            goto error;
        }

#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
        py_nic_info = Py_BuildValue("(KKKKKKKK)",
                                    pIfRow->OutOctets,
                                    pIfRow->InOctets,
                                    (pIfRow->OutUcastPkts + pIfRow->OutNUcastPkts),
                                    (pIfRow->InUcastPkts + pIfRow->InNUcastPkts),
                                    pIfRow->InErrors,
                                    pIfRow->OutErrors,
                                    pIfRow->InDiscards,
                                    pIfRow->OutDiscards);
#else // Windows XP
        py_nic_info = Py_BuildValue("(kkkkkkkk)",
                                    pIfRow->dwOutOctets,
                                    pIfRow->dwInOctets,
                                    (pIfRow->dwOutUcastPkts + pIfRow->dwOutNUcastPkts),
                                    (pIfRow->dwInUcastPkts + pIfRow->dwInNUcastPkts),
                                    pIfRow->dwInErrors,
                                    pIfRow->dwOutErrors,
                                    pIfRow->dwInDiscards,
                                    pIfRow->dwOutDiscards);
#endif

        if (!py_nic_info)
            goto error;

        py_nic_name = PyUnicode_FromWideChar(
            pCurrAddresses->FriendlyName,
            wcslen(pCurrAddresses->FriendlyName));

        if (py_nic_name == NULL)
            goto error;
        if (PyDict_SetItem(py_retdict, py_nic_name, py_nic_info))
            goto error;
        Py_XDECREF(py_nic_name);
        Py_XDECREF(py_nic_info);

        free(pIfRow);
        pCurrAddresses = pCurrAddresses->Next;
    }

    free(pAddresses);
    return py_retdict;

error:
    Py_XDECREF(py_nic_name);
    Py_XDECREF(py_nic_info);
    Py_DECREF(py_retdict);
    if (pAddresses != NULL)
        free(pAddresses);
    if (pIfRow != NULL)
        free(pIfRow);
    return NULL;
}