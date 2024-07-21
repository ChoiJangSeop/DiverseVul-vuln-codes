psutil_disk_io_counters(PyObject *self, PyObject *args) {
    DISK_PERFORMANCE diskPerformance;
    DWORD dwSize;
    HANDLE hDevice = NULL;
    char szDevice[MAX_PATH];
    char szDeviceDisplay[MAX_PATH];
    int devNum;
    int i;
    DWORD ioctrlSize;
    BOOL ret;
    PyObject *py_retdict = PyDict_New();
    PyObject *py_tuple = NULL;

    if (py_retdict == NULL)
        return NULL;
    // Apparently there's no way to figure out how many times we have
    // to iterate in order to find valid drives.
    // Let's assume 32, which is higher than 26, the number of letters
    // in the alphabet (from A:\ to Z:\).
    for (devNum = 0; devNum <= 32; ++devNum) {
        py_tuple = NULL;
        sprintf_s(szDevice, MAX_PATH, "\\\\.\\PhysicalDrive%d", devNum);
        hDevice = CreateFile(szDevice, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, 0, NULL);
        if (hDevice == INVALID_HANDLE_VALUE)
            continue;

        // DeviceIoControl() sucks!
        i = 0;
        ioctrlSize = sizeof(diskPerformance);
        while (1) {
            i += 1;
            ret = DeviceIoControl(
                hDevice, IOCTL_DISK_PERFORMANCE, NULL, 0, &diskPerformance,
                ioctrlSize, &dwSize, NULL);
            if (ret != 0)
                break;  // OK!
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                // Retry with a bigger buffer (+ limit for retries).
                if (i <= 1024) {
                    ioctrlSize *= 2;
                    continue;
                }
            }
            else if (GetLastError() == ERROR_INVALID_FUNCTION) {
                // This happens on AppVeyor:
                // https://ci.appveyor.com/project/giampaolo/psutil/build/
                //      1364/job/ascpdi271b06jle3
                // Assume it means we're dealing with some exotic disk
                // and go on.
                psutil_debug("DeviceIoControl -> ERROR_INVALID_FUNCTION; "
                             "ignore PhysicalDrive%i", devNum);
                goto next;
            }
            else if (GetLastError() == ERROR_NOT_SUPPORTED) {
                // Again, let's assume we're dealing with some exotic disk.
                psutil_debug("DeviceIoControl -> ERROR_NOT_SUPPORTED; "
                             "ignore PhysicalDrive%i", devNum);
                goto next;
            }
            // XXX: it seems we should also catch ERROR_INVALID_PARAMETER:
            // https://sites.ualberta.ca/dept/aict/uts/software/openbsd/
            //     ports/4.1/i386/openafs/w-openafs-1.4.14-transarc/
            //     openafs-1.4.14/src/usd/usd_nt.c

            // XXX: we can also bump into ERROR_MORE_DATA in which case
            // (quoting doc) we're supposed to retry with a bigger buffer
            // and specify  a new "starting point", whatever it means.
            PyErr_SetFromWindowsErr(0);
            goto error;
        }

        sprintf_s(szDeviceDisplay, MAX_PATH, "PhysicalDrive%i", devNum);
        py_tuple = Py_BuildValue(
            "(IILLKK)",
            diskPerformance.ReadCount,
            diskPerformance.WriteCount,
            diskPerformance.BytesRead,
            diskPerformance.BytesWritten,
            // convert to ms:
            // https://github.com/giampaolo/psutil/issues/1012
            (unsigned long long)
                (diskPerformance.ReadTime.QuadPart) / 10000000,
            (unsigned long long)
                (diskPerformance.WriteTime.QuadPart) / 10000000);
        if (!py_tuple)
            goto error;
        if (PyDict_SetItemString(py_retdict, szDeviceDisplay, py_tuple))
            goto error;
        Py_XDECREF(py_tuple);

next:
        CloseHandle(hDevice);
    }

    return py_retdict;

error:
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retdict);
    if (hDevice != NULL)
        CloseHandle(hDevice);
    return NULL;
}