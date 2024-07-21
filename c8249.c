psutil_net_if_addrs(PyObject *self, PyObject *args) {
    unsigned int i = 0;
    ULONG family;
    PCTSTR intRet;
    PCTSTR netmaskIntRet;
    char *ptr;
    char buff_addr[1024];
    char buff_macaddr[1024];
    char buff_netmask[1024];
    DWORD dwRetVal = 0;
#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
    ULONG converted_netmask;
    UINT netmask_bits;
    struct in_addr in_netmask;
#endif
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;

    PyObject *py_retlist = PyList_New(0);
    PyObject *py_tuple = NULL;
    PyObject *py_address = NULL;
    PyObject *py_mac_address = NULL;
    PyObject *py_nic_name = NULL;
    PyObject *py_netmask = NULL;

    if (py_retlist == NULL)
        return NULL;

    pAddresses = psutil_get_nic_addresses();
    if (pAddresses == NULL)
        goto error;
    pCurrAddresses = pAddresses;

    while (pCurrAddresses) {
        pUnicast = pCurrAddresses->FirstUnicastAddress;

        netmaskIntRet = NULL;
        py_nic_name = NULL;
        py_nic_name = PyUnicode_FromWideChar(
            pCurrAddresses->FriendlyName,
            wcslen(pCurrAddresses->FriendlyName));
        if (py_nic_name == NULL)
            goto error;

        // MAC address
        if (pCurrAddresses->PhysicalAddressLength != 0) {
            ptr = buff_macaddr;
            *ptr = '\0';
            for (i = 0; i < (int) pCurrAddresses->PhysicalAddressLength; i++) {
                if (i == (pCurrAddresses->PhysicalAddressLength - 1)) {
                    sprintf_s(ptr, _countof(buff_macaddr), "%.2X\n",
                            (int)pCurrAddresses->PhysicalAddress[i]);
                }
                else {
                    sprintf_s(ptr, _countof(buff_macaddr), "%.2X-",
                            (int)pCurrAddresses->PhysicalAddress[i]);
                }
                ptr += 3;
            }
            *--ptr = '\0';

            py_mac_address = Py_BuildValue("s", buff_macaddr);
            if (py_mac_address == NULL)
                goto error;

            Py_INCREF(Py_None);
            Py_INCREF(Py_None);
            Py_INCREF(Py_None);
            py_tuple = Py_BuildValue(
                "(OiOOOO)",
                py_nic_name,
                -1,  // this will be converted later to AF_LINK
                py_mac_address,
                Py_None,  // netmask (not supported)
                Py_None,  // broadcast (not supported)
                Py_None  // ptp (not supported on Windows)
            );
            if (! py_tuple)
                goto error;
            if (PyList_Append(py_retlist, py_tuple))
                goto error;
            Py_DECREF(py_tuple);
            Py_DECREF(py_mac_address);
        }

        // find out the IP address associated with the NIC
        if (pUnicast != NULL) {
            for (i = 0; pUnicast != NULL; i++) {
                family = pUnicast->Address.lpSockaddr->sa_family;
                if (family == AF_INET) {
                    struct sockaddr_in *sa_in = (struct sockaddr_in *)
                        pUnicast->Address.lpSockaddr;
                    intRet = inet_ntop(AF_INET, &(sa_in->sin_addr), buff_addr,
                                       sizeof(buff_addr));
                    if (!intRet)
                        goto error;
#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
                    netmask_bits = pUnicast->OnLinkPrefixLength;
                    dwRetVal = ConvertLengthToIpv4Mask(netmask_bits, &converted_netmask);
                    if (dwRetVal == NO_ERROR) {
                        in_netmask.s_addr = converted_netmask;
                        netmaskIntRet = inet_ntop(
                            AF_INET, &in_netmask, buff_netmask,
                            sizeof(buff_netmask));
                        if (!netmaskIntRet)
                            goto error;
                    }
#endif
                }
                else if (family == AF_INET6) {
                    struct sockaddr_in6 *sa_in6 = (struct sockaddr_in6 *)
                        pUnicast->Address.lpSockaddr;
                    intRet = inet_ntop(AF_INET6, &(sa_in6->sin6_addr),
                                       buff_addr, sizeof(buff_addr));
                    if (!intRet)
                        goto error;
                }
                else {
                    // we should never get here
                    pUnicast = pUnicast->Next;
                    continue;
                }

#if PY_MAJOR_VERSION >= 3
                py_address = PyUnicode_FromString(buff_addr);
#else
                py_address = PyString_FromString(buff_addr);
#endif
                if (py_address == NULL)
                    goto error;

                if (netmaskIntRet != NULL) {
#if PY_MAJOR_VERSION >= 3
                    py_netmask = PyUnicode_FromString(buff_netmask);
#else
                    py_netmask = PyString_FromString(buff_netmask);
#endif
                } else {
                    Py_INCREF(Py_None);
                    py_netmask = Py_None;
                }

                Py_INCREF(Py_None);
                Py_INCREF(Py_None);
                py_tuple = Py_BuildValue(
                    "(OiOOOO)",
                    py_nic_name,
                    family,
                    py_address,
                    py_netmask,
                    Py_None,  // broadcast (not supported)
                    Py_None  // ptp (not supported on Windows)
                );

                if (! py_tuple)
                    goto error;
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
                Py_DECREF(py_address);
                Py_DECREF(py_netmask);

                pUnicast = pUnicast->Next;
            }
        }
        Py_DECREF(py_nic_name);
        pCurrAddresses = pCurrAddresses->Next;
    }

    free(pAddresses);
    return py_retlist;

error:
    if (pAddresses)
        free(pAddresses);
    Py_DECREF(py_retlist);
    Py_XDECREF(py_tuple);
    Py_XDECREF(py_address);
    Py_XDECREF(py_nic_name);
    Py_XDECREF(py_netmask);
    return NULL;
}