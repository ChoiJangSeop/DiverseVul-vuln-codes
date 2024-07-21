psutil_disk_io_counters(PyObject *self, PyObject *args) {
    CFDictionaryRef parent_dict;
    CFDictionaryRef props_dict;
    CFDictionaryRef stats_dict;
    io_registry_entry_t parent;
    io_registry_entry_t disk;
    io_iterator_t disk_list;
    PyObject *py_disk_info = NULL;
    PyObject *py_retdict = PyDict_New();

    if (py_retdict == NULL)
        return NULL;

    // Get list of disks
    if (IOServiceGetMatchingServices(kIOMasterPortDefault,
                                     IOServiceMatching(kIOMediaClass),
                                     &disk_list) != kIOReturnSuccess) {
        PyErr_SetString(
            PyExc_RuntimeError, "unable to get the list of disks.");
        goto error;
    }

    // Iterate over disks
    while ((disk = IOIteratorNext(disk_list)) != 0) {
        py_disk_info = NULL;
        parent_dict = NULL;
        props_dict = NULL;
        stats_dict = NULL;

        if (IORegistryEntryGetParentEntry(disk, kIOServicePlane, &parent)
                != kIOReturnSuccess) {
            PyErr_SetString(PyExc_RuntimeError,
                            "unable to get the disk's parent.");
            IOObjectRelease(disk);
            goto error;
        }

        if (IOObjectConformsTo(parent, "IOBlockStorageDriver")) {
            if (IORegistryEntryCreateCFProperties(
                    disk,
                    (CFMutableDictionaryRef *) &parent_dict,
                    kCFAllocatorDefault,
                    kNilOptions
                ) != kIOReturnSuccess)
            {
                PyErr_SetString(PyExc_RuntimeError,
                                "unable to get the parent's properties.");
                IOObjectRelease(disk);
                IOObjectRelease(parent);
                goto error;
            }

            if (IORegistryEntryCreateCFProperties(
                    parent,
                    (CFMutableDictionaryRef *) &props_dict,
                    kCFAllocatorDefault,
                    kNilOptions
                ) != kIOReturnSuccess)
            {
                PyErr_SetString(PyExc_RuntimeError,
                                "unable to get the disk properties.");
                CFRelease(props_dict);
                IOObjectRelease(disk);
                IOObjectRelease(parent);
                goto error;
            }

            const int kMaxDiskNameSize = 64;
            CFStringRef disk_name_ref = (CFStringRef)CFDictionaryGetValue(
                parent_dict, CFSTR(kIOBSDNameKey));
            char disk_name[kMaxDiskNameSize];

            CFStringGetCString(disk_name_ref,
                               disk_name,
                               kMaxDiskNameSize,
                               CFStringGetSystemEncoding());

            stats_dict = (CFDictionaryRef)CFDictionaryGetValue(
                props_dict, CFSTR(kIOBlockStorageDriverStatisticsKey));

            if (stats_dict == NULL) {
                PyErr_SetString(PyExc_RuntimeError,
                                "Unable to get disk stats.");
                goto error;
            }

            CFNumberRef number;
            int64_t reads = 0;
            int64_t writes = 0;
            int64_t read_bytes = 0;
            int64_t write_bytes = 0;
            int64_t read_time = 0;
            int64_t write_time = 0;

            // Get disk reads/writes
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsReadsKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &reads);
            }
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsWritesKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &writes);
            }

            // Get disk bytes read/written
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsBytesReadKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &read_bytes);
            }
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsBytesWrittenKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &write_bytes);
            }

            // Get disk time spent reading/writing (nanoseconds)
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsTotalReadTimeKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &read_time);
            }
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsTotalWriteTimeKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &write_time);
            }

            // Read/Write time on macOS comes back in nanoseconds and in psutil
            // we've standardized on milliseconds so do the conversion.
            py_disk_info = Py_BuildValue(
                "(KKKKKK)",
                reads,
                writes,
                read_bytes,
                write_bytes,
                read_time / 1000 / 1000,
                write_time / 1000 / 1000);
           if (!py_disk_info)
                goto error;
            if (PyDict_SetItemString(py_retdict, disk_name, py_disk_info))
                goto error;
            Py_DECREF(py_disk_info);

            CFRelease(parent_dict);
            IOObjectRelease(parent);
            CFRelease(props_dict);
            IOObjectRelease(disk);
        }
    }

    IOObjectRelease (disk_list);

    return py_retdict;

error:
    Py_XDECREF(py_disk_info);
    Py_DECREF(py_retdict);
    return NULL;
}