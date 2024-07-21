PyImaging_LibTiffEncoderNew(PyObject* self, PyObject* args)
{
    ImagingEncoderObject* encoder;

    char* mode;
    char* rawmode;
    char* compname;
    char* filename;
    int compression;
    int fp;

    PyObject *dir;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    int status;

    Py_ssize_t d_size;
    PyObject *keys, *values;


    if (! PyArg_ParseTuple(args, "sssisO", &mode, &rawmode, &compname, &fp, &filename, &dir)) {
        return NULL;
    }

    if (!PyDict_Check(dir)) {
        PyErr_SetString(PyExc_ValueError, "Invalid Dictionary");
        return NULL;
    } else {
        d_size = PyDict_Size(dir);
        TRACE(("dict size: %d\n", (int)d_size));
        keys = PyDict_Keys(dir);
        values = PyDict_Values(dir);
        for (pos=0;pos<d_size;pos++){
            TRACE(("  key: %d\n", (int)PyInt_AsLong(PyList_GetItem(keys,pos))));
        }
        pos = 0;
    }


    TRACE(("new tiff encoder %s fp: %d, filename: %s \n", compname, fp, filename));

    /* UNDONE -- we can probably do almost any arbitrary compression here,
     *  so long as we're doing row/stripe based actions and not tiles.
     */

    if (strcasecmp(compname, "tiff_ccitt") == 0) {
        compression = COMPRESSION_CCITTRLE;

    } else if (strcasecmp(compname, "group3") == 0) {
        compression = COMPRESSION_CCITTFAX3;

    } else if (strcasecmp(compname, "group4") == 0) {
        compression = COMPRESSION_CCITTFAX4;

    } else if (strcasecmp(compname, "tiff_raw_16") == 0) {
        compression = COMPRESSION_CCITTRLEW;

    } else {
        PyErr_SetString(PyExc_ValueError, "unknown compession");
        return NULL;
    }

    TRACE(("Found compression: %d\n", compression));

    encoder = PyImaging_EncoderNew(sizeof(TIFFSTATE));
    if (encoder == NULL)
        return NULL;

    if (get_packer(encoder, mode, rawmode) < 0)
        return NULL;

    if (! ImagingLibTiffEncodeInit(&encoder->state, filename, fp)) {
        Py_DECREF(encoder);
        PyErr_SetString(PyExc_RuntimeError, "tiff codec initialization failed");
        return NULL;
    }

	// While failes on 64 bit machines, complains that pos is an int instead of a Py_ssize_t
	//    while (PyDict_Next(dir, &pos, &key, &value)) {
	for (pos=0;pos<d_size;pos++){
		key = PyList_GetItem(keys,pos);
		value = PyList_GetItem(values,pos);
        status = 0;
        TRACE(("Attempting to set key: %d\n", (int)PyInt_AsLong(key)));
        if (PyInt_Check(value)) {
            TRACE(("Setting from Int: %d %ld \n", (int)PyInt_AsLong(key),PyInt_AsLong(value)));
            status = ImagingLibTiffSetField(&encoder->state,
                                            (ttag_t) PyInt_AsLong(key),
                                            PyInt_AsLong(value));
        } else if(PyBytes_Check(value)) {
            TRACE(("Setting from String: %d, %s \n", (int)PyInt_AsLong(key),PyBytes_AsString(value)));
            status = ImagingLibTiffSetField(&encoder->state,
                                            (ttag_t) PyInt_AsLong(key),
                                            PyBytes_AsString(value));

        } else if(PyList_Check(value)) {
            int len,i;
            float *floatav;
            TRACE(("Setting from List: %d \n", (int)PyInt_AsLong(key)));
            len = (int)PyList_Size(value);
            TRACE((" %d elements, setting as floats \n", len));
            floatav = malloc(sizeof(float)*len);
            if (floatav) {
                for (i=0;i<len;i++) {
                    floatav[i] = (float)PyFloat_AsDouble(PyList_GetItem(value,i));
                }
                status = ImagingLibTiffSetField(&encoder->state,
                                                (ttag_t) PyInt_AsLong(key),
                                                floatav);
                free(floatav);
            }
        } else if (PyFloat_Check(value)) {
            TRACE(("Setting from String: %d, %f \n", (int)PyInt_AsLong(key),PyFloat_AsDouble(value)));
            status = ImagingLibTiffSetField(&encoder->state,
                                            (ttag_t) PyInt_AsLong(key),
                                            (float)PyFloat_AsDouble(value));
        } else {
            TRACE(("Unhandled type for key %d : %s ",
                   (int)PyInt_AsLong(key),
                   PyBytes_AsString(PyObject_Str(value))));
        }
        if (!status) {
            TRACE(("Error setting Field\n"));
            Py_DECREF(encoder);
            PyErr_SetString(PyExc_RuntimeError, "Error setting from dictionary");
            return NULL;
        }
    }

    encoder->encode  = ImagingLibTiffEncode;

    return (PyObject*) encoder;
}