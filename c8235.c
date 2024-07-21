psutil_disk_partitions(PyObject *self, PyObject *args) {
    DWORD num_bytes;
    char drive_strings[255];
    char *drive_letter = drive_strings;
    char mp_buf[MAX_PATH];
    char mp_path[MAX_PATH];
    int all;
    int type;
    int ret;
    unsigned int old_mode = 0;
    char opts[20];
    HANDLE mp_h;
    BOOL mp_flag= TRUE;
    LPTSTR fs_type[MAX_PATH + 1] = { 0 };
    DWORD pflags = 0;
    PyObject *py_all;
    PyObject *py_retlist = PyList_New(0);
    PyObject *py_tuple = NULL;

    if (py_retlist == NULL) {
        return NULL;
    }

    // avoid to visualize a message box in case something goes wrong
    // see https://github.com/giampaolo/psutil/issues/264
    old_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

    if (! PyArg_ParseTuple(args, "O", &py_all))
        goto error;
    all = PyObject_IsTrue(py_all);

    Py_BEGIN_ALLOW_THREADS
    num_bytes = GetLogicalDriveStrings(254, drive_letter);
    Py_END_ALLOW_THREADS

    if (num_bytes == 0) {
        PyErr_SetFromWindowsErr(0);
        goto error;
    }

    while (*drive_letter != 0) {
        py_tuple = NULL;
        opts[0] = 0;
        fs_type[0] = 0;

        Py_BEGIN_ALLOW_THREADS
        type = GetDriveType(drive_letter);
        Py_END_ALLOW_THREADS

        // by default we only show hard drives and cd-roms
        if (all == 0) {
            if ((type == DRIVE_UNKNOWN) ||
                    (type == DRIVE_NO_ROOT_DIR) ||
                    (type == DRIVE_REMOTE) ||
                    (type == DRIVE_RAMDISK)) {
                goto next;
            }
            // floppy disk: skip it by default as it introduces a
            // considerable slowdown.
            if ((type == DRIVE_REMOVABLE) &&
                    (strcmp(drive_letter, "A:\\")  == 0)) {
                goto next;
            }
        }

        ret = GetVolumeInformation(
            (LPCTSTR)drive_letter, NULL, _ARRAYSIZE(drive_letter),
            NULL, NULL, &pflags, (LPTSTR)fs_type, _ARRAYSIZE(fs_type));
        if (ret == 0) {
            // We might get here in case of a floppy hard drive, in
            // which case the error is (21, "device not ready").
            // Let's pretend it didn't happen as we already have
            // the drive name and type ('removable').
            strcat_s(opts, _countof(opts), "");
            SetLastError(0);
        }
        else {
            if (pflags & FILE_READ_ONLY_VOLUME)
                strcat_s(opts, _countof(opts), "ro");
            else
                strcat_s(opts, _countof(opts), "rw");
            if (pflags & FILE_VOLUME_IS_COMPRESSED)
                strcat_s(opts, _countof(opts), ",compressed");

            // Check for mount points on this volume and add/get info
            // (checks first to know if we can even have mount points)
            if (pflags & FILE_SUPPORTS_REPARSE_POINTS) {

                mp_h = FindFirstVolumeMountPoint(drive_letter, mp_buf, MAX_PATH);
                if (mp_h != INVALID_HANDLE_VALUE) {
                    while (mp_flag) {

                        // Append full mount path with drive letter
                        strcpy_s(mp_path, _countof(mp_path), drive_letter);
                        strcat_s(mp_path, _countof(mp_path), mp_buf);

                        py_tuple = Py_BuildValue(
                            "(ssss)",
                            drive_letter,
                            mp_path,
                            fs_type, // Typically NTFS
                            opts);

                        if (!py_tuple || PyList_Append(py_retlist, py_tuple) == -1) {
                            FindVolumeMountPointClose(mp_h);
                            goto error;
                        }

                        Py_DECREF(py_tuple);

                        // Continue looking for more mount points
                        mp_flag = FindNextVolumeMountPoint(mp_h, mp_buf, MAX_PATH);
                    }
                    FindVolumeMountPointClose(mp_h);
                }

            }
        }

        if (strlen(opts) > 0)
            strcat_s(opts, _countof(opts), ",");
        strcat_s(opts, _countof(opts), psutil_get_drive_type(type));

        py_tuple = Py_BuildValue(
            "(ssss)",
            drive_letter,
            drive_letter,
            fs_type,  // either FAT, FAT32, NTFS, HPFS, CDFS, UDF or NWFS
            opts);
        if (!py_tuple)
            goto error;
        if (PyList_Append(py_retlist, py_tuple))
            goto error;
        Py_DECREF(py_tuple);
        goto next;

next:
        drive_letter = strchr(drive_letter, 0) + 1;
    }

    SetErrorMode(old_mode);
    return py_retlist;

error:
    SetErrorMode(old_mode);
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retlist);
    return NULL;
}