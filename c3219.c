do_curl_setopt(CurlObject *self, PyObject *args)
{
    int option;
    PyObject *obj;
    int res;
    PyObject *encoded_obj;

    if (!PyArg_ParseTuple(args, "iO:setopt", &option, &obj))
        return NULL;
    if (check_curl_state(self, 1 | 2, "setopt") != 0)
        return NULL;

    /* early checks of option value */
    if (option <= 0)
        goto error;
    if (option >= (int)CURLOPTTYPE_OFF_T + OPTIONS_SIZE)
        goto error;
    if (option % 10000 >= OPTIONS_SIZE)
        goto error;

    /* Handle the case of None as the call of unsetopt() */
    if (obj == Py_None) {
        return util_curl_unsetopt(self, option);
    }

    /* Handle the case of string arguments */

    if (PyText_Check(obj)) {
        char *str = NULL;
        Py_ssize_t len = -1;

        /* Check that the option specified a string as well as the input */
        switch (option) {
        case CURLOPT_CAINFO:
        case CURLOPT_CAPATH:
        case CURLOPT_COOKIE:
        case CURLOPT_COOKIEFILE:
        case CURLOPT_COOKIELIST:
        case CURLOPT_COOKIEJAR:
        case CURLOPT_CUSTOMREQUEST:
        case CURLOPT_EGDSOCKET:
        case CURLOPT_ENCODING:
        case CURLOPT_FTPPORT:
        case CURLOPT_INTERFACE:
        case CURLOPT_KRB4LEVEL:
        case CURLOPT_NETRC_FILE:
        case CURLOPT_PROXY:
        case CURLOPT_PROXYUSERPWD:
#ifdef HAVE_CURLOPT_PROXYUSERNAME
        case CURLOPT_PROXYUSERNAME:
        case CURLOPT_PROXYPASSWORD:
#endif
        case CURLOPT_RANDOM_FILE:
        case CURLOPT_RANGE:
        case CURLOPT_REFERER:
        case CURLOPT_SSLCERT:
        case CURLOPT_SSLCERTTYPE:
        case CURLOPT_SSLENGINE:
        case CURLOPT_SSLKEY:
        case CURLOPT_SSLKEYPASSWD:
        case CURLOPT_SSLKEYTYPE:
        case CURLOPT_SSL_CIPHER_LIST:
        case CURLOPT_URL:
        case CURLOPT_USERAGENT:
        case CURLOPT_USERPWD:
#ifdef HAVE_CURLOPT_USERNAME
        case CURLOPT_USERNAME:
        case CURLOPT_PASSWORD:
#endif
        case CURLOPT_FTP_ALTERNATIVE_TO_USER:
        case CURLOPT_SSH_PUBLIC_KEYFILE:
        case CURLOPT_SSH_PRIVATE_KEYFILE:
        case CURLOPT_COPYPOSTFIELDS:
        case CURLOPT_SSH_HOST_PUBLIC_KEY_MD5:
        case CURLOPT_CRLFILE:
        case CURLOPT_ISSUERCERT:
#ifdef HAVE_CURLOPT_DNS_SERVERS
        case CURLOPT_DNS_SERVERS:
#endif
#ifdef HAVE_CURLOPT_NOPROXY
        case CURLOPT_NOPROXY:
#endif
#ifdef HAVE_CURL_7_19_4_OPTS
        case CURLOPT_SOCKS5_GSSAPI_SERVICE:
#endif
#ifdef HAVE_CURL_7_19_6_OPTS
        case CURLOPT_SSH_KNOWNHOSTS:
#endif
#ifdef HAVE_CURL_7_20_0_OPTS
        case CURLOPT_MAIL_FROM:
#endif
#ifdef HAVE_CURL_7_25_0_OPTS
        case CURLOPT_MAIL_AUTH:
#endif
#if LIBCURL_VERSION_NUM >= 0x072b00 /* check for 7.43.0 or greater */
    case CURLOPT_SERVICE_NAME:
    case CURLOPT_PROXY_SERVICE_NAME:
#endif
/* FIXME: check if more of these options allow binary data */
            str = PyText_AsString_NoNUL(obj, &encoded_obj);
            if (str == NULL)
                return NULL;
            break;
        case CURLOPT_POSTFIELDS:
            if (PyText_AsStringAndSize(obj, &str, &len, &encoded_obj) != 0)
                return NULL;
            /* automatically set POSTFIELDSIZE */
            if (len <= INT_MAX) {
                res = curl_easy_setopt(self->handle, CURLOPT_POSTFIELDSIZE, (long)len);
            } else {
                res = curl_easy_setopt(self->handle, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)len);
            }
            if (res != CURLE_OK) {
                PyText_EncodedDecref(encoded_obj);
                CURLERROR_RETVAL();
            }
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "strings are not supported for this option");
            return NULL;
        }
        assert(str != NULL);
        /* Call setopt */
        res = curl_easy_setopt(self->handle, (CURLoption)option, str);
        /* Check for errors */
        if (res != CURLE_OK) {
            PyText_EncodedDecref(encoded_obj);
            CURLERROR_RETVAL();
        }
        /* libcurl does not copy the value of CURLOPT_POSTFIELDS */
        if (option == CURLOPT_POSTFIELDS) {
            PyObject *store_obj;

            /* if obj was bytes, it was not encoded, and we need to incref obj.
             * if obj was unicode, it was encoded, and we need to incref
             * encoded_obj - except we can simply transfer ownership.
             */
            if (encoded_obj) {
                store_obj = encoded_obj;
            } else {
                /* no encoding is performed, incref the original object. */
                store_obj = obj;
                Py_INCREF(store_obj);
            }

            util_curl_xdecref(self, PYCURL_MEMGROUP_POSTFIELDS, self->handle);
            self->postfields_obj = store_obj;
        } else {
            PyText_EncodedDecref(encoded_obj);
        }
        Py_RETURN_NONE;
    }

#define IS_LONG_OPTION(o)   (o < CURLOPTTYPE_OBJECTPOINT)
#define IS_OFF_T_OPTION(o)  (o >= CURLOPTTYPE_OFF_T)

    /* Handle the case of integer arguments */
    if (PyInt_Check(obj)) {
        long d = PyInt_AsLong(obj);

        if (IS_LONG_OPTION(option))
            res = curl_easy_setopt(self->handle, (CURLoption)option, (long)d);
        else if (IS_OFF_T_OPTION(option))
            res = curl_easy_setopt(self->handle, (CURLoption)option, (curl_off_t)d);
        else {
            PyErr_SetString(PyExc_TypeError, "integers are not supported for this option");
            return NULL;
        }
        if (res != CURLE_OK) {
            CURLERROR_RETVAL();
        }
        Py_RETURN_NONE;
    }

    /* Handle the case of long arguments (used by *_LARGE options) */
    if (PyLong_Check(obj)) {
        PY_LONG_LONG d = PyLong_AsLongLong(obj);
        if (d == -1 && PyErr_Occurred())
            return NULL;

        if (IS_LONG_OPTION(option) && (long)d == d)
            res = curl_easy_setopt(self->handle, (CURLoption)option, (long)d);
        else if (IS_OFF_T_OPTION(option) && (curl_off_t)d == d)
            res = curl_easy_setopt(self->handle, (CURLoption)option, (curl_off_t)d);
        else {
            PyErr_SetString(PyExc_TypeError, "longs are not supported for this option");
            return NULL;
        }
        if (res != CURLE_OK) {
            CURLERROR_RETVAL();
        }
        Py_RETURN_NONE;
    }

#undef IS_LONG_OPTION
#undef IS_OFF_T_OPTION

#if PY_MAJOR_VERSION < 3 && !defined(PYCURL_AVOID_STDIO)
    /* Handle the case of file objects */
    if (PyFile_Check(obj)) {
        FILE *fp;

        /* Ensure the option specified a file as well as the input */
        switch (option) {
        case CURLOPT_READDATA:
        case CURLOPT_WRITEDATA:
            break;
        case CURLOPT_WRITEHEADER:
            if (self->w_cb != NULL) {
                PyErr_SetString(ErrorObject, "cannot combine WRITEHEADER with WRITEFUNCTION.");
                return NULL;
            }
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "files are not supported for this option");
            return NULL;
        }

        fp = PyFile_AsFile(obj);
        if (fp == NULL) {
            PyErr_SetString(PyExc_TypeError, "second argument must be open file");
            return NULL;
        }
        res = curl_easy_setopt(self->handle, (CURLoption)option, fp);
        if (res != CURLE_OK) {
            CURLERROR_RETVAL();
        }
        Py_INCREF(obj);

        switch (option) {
        case CURLOPT_READDATA:
            Py_CLEAR(self->readdata_fp);
            self->readdata_fp = obj;
            break;
        case CURLOPT_WRITEDATA:
            Py_CLEAR(self->writedata_fp);
            self->writedata_fp = obj;
            break;
        case CURLOPT_WRITEHEADER:
            Py_CLEAR(self->writeheader_fp);
            self->writeheader_fp = obj;
            break;
        default:
            assert(0);
            break;
        }
        /* Return success */
        Py_RETURN_NONE;
    }
#endif

    /* Handle the case of list objects */
    if (PyList_Check(obj)) {
        struct curl_slist **old_slist = NULL;
        struct curl_slist *slist = NULL;
        Py_ssize_t i, len;

        switch (option) {
        case CURLOPT_HTTP200ALIASES:
            old_slist = &self->http200aliases;
            break;
        case CURLOPT_HTTPHEADER:
            old_slist = &self->httpheader;
            break;
        case CURLOPT_POSTQUOTE:
            old_slist = &self->postquote;
            break;
        case CURLOPT_PREQUOTE:
            old_slist = &self->prequote;
            break;
        case CURLOPT_QUOTE:
            old_slist = &self->quote;
            break;
#ifdef HAVE_CURLOPT_RESOLVE
        case CURLOPT_RESOLVE:
            old_slist = &self->resolve;
            break;
#endif
#ifdef HAVE_CURL_7_20_0_OPTS
        case CURLOPT_MAIL_RCPT:
            old_slist = &self->mail_rcpt;
            break;
#endif
        case CURLOPT_HTTPPOST:
            break;
        default:
            /* None of the list options were recognized, raise exception */
            PyErr_SetString(PyExc_TypeError, "lists are not supported for this option");
            return NULL;
        }

        len = PyList_Size(obj);
        if (len == 0)
            Py_RETURN_NONE;

        /* Handle HTTPPOST different since we construct a HttpPost form struct */
        if (option == CURLOPT_HTTPPOST) {
            struct curl_httppost *post = NULL;
            struct curl_httppost *last = NULL;
            /* List of all references that have been INCed as a result of
             * this operation */
            PyObject *ref_params = NULL;
            PyObject *nencoded_obj, *cencoded_obj, *oencoded_obj;

            for (i = 0; i < len; i++) {
                char *nstr = NULL, *cstr = NULL;
                Py_ssize_t nlen = -1, clen = -1;
                PyObject *listitem = PyList_GetItem(obj, i);

                if (!PyTuple_Check(listitem)) {
                    curl_formfree(post);
                    Py_XDECREF(ref_params);
                    PyErr_SetString(PyExc_TypeError, "list items must be tuple objects");
                    return NULL;
                }
                if (PyTuple_GET_SIZE(listitem) != 2) {
                    curl_formfree(post);
                    Py_XDECREF(ref_params);
                    PyErr_SetString(PyExc_TypeError, "tuple must contain two elements (name, value)");
                    return NULL;
                }
                if (PyText_AsStringAndSize(PyTuple_GET_ITEM(listitem, 0), &nstr, &nlen, &nencoded_obj) != 0) {
                    curl_formfree(post);
                    Py_XDECREF(ref_params);
                    PyErr_SetString(PyExc_TypeError, "tuple must contain a byte string or Unicode string with ASCII code points only as first element");
                    return NULL;
                }
                if (PyText_Check(PyTuple_GET_ITEM(listitem, 1))) {
                    /* Handle strings as second argument for backwards compatibility */

                    if (PyText_AsStringAndSize(PyTuple_GET_ITEM(listitem, 1), &cstr, &clen, &cencoded_obj)) {
                        curl_formfree(post);
                        Py_XDECREF(ref_params);
                        CURLERROR_RETVAL();
                    }
                    /* INFO: curl_formadd() internally does memdup() the data, so
                     * embedded NUL characters _are_ allowed here. */
                    res = curl_formadd(&post, &last,
                                       CURLFORM_COPYNAME, nstr,
                                       CURLFORM_NAMELENGTH, (long) nlen,
                                       CURLFORM_COPYCONTENTS, cstr,
                                       CURLFORM_CONTENTSLENGTH, (long) clen,
                                       CURLFORM_END);
                    PyText_EncodedDecref(cencoded_obj);
                    if (res != CURLE_OK) {
                        curl_formfree(post);
                        Py_XDECREF(ref_params);
                        CURLERROR_RETVAL();
                    }
                }
                else if (PyTuple_Check(PyTuple_GET_ITEM(listitem, 1))) {
                    /* Supports content, file and content-type */
                    PyObject *t = PyTuple_GET_ITEM(listitem, 1);
                    Py_ssize_t tlen = PyTuple_Size(t);
                    int j, k, l;
                    struct curl_forms *forms = NULL;

                    /* Sanity check that there are at least two tuple items */
                    if (tlen < 2) {
                        curl_formfree(post);
                        Py_XDECREF(ref_params);
                        PyErr_SetString(PyExc_TypeError, "tuple must contain at least one option and one value");
                        return NULL;
                    }

                    /* Allocate enough space to accommodate length options for content or buffers, plus a terminator. */
                    forms = PyMem_Malloc(sizeof(struct curl_forms) * ((tlen*2) + 1));
                    if (forms == NULL) {
                        curl_formfree(post);
                        Py_XDECREF(ref_params);
                        PyErr_NoMemory();
                        return NULL;
                    }

                    /* Iterate all the tuple members pairwise */
                    for (j = 0, k = 0, l = 0; j < tlen; j += 2, l++) {
                        char *ostr;
                        Py_ssize_t olen;
                        int val;

                        if (j == (tlen-1)) {
                            PyErr_SetString(PyExc_TypeError, "expected value");
                            PyMem_Free(forms);
                            curl_formfree(post);
                            Py_XDECREF(ref_params);
                            return NULL;
                        }
                        if (!PyInt_Check(PyTuple_GET_ITEM(t, j))) {
                            PyErr_SetString(PyExc_TypeError, "option must be long");
                            PyMem_Free(forms);
                            curl_formfree(post);
                            Py_XDECREF(ref_params);
                            return NULL;
                        }
                        if (!PyText_Check(PyTuple_GET_ITEM(t, j+1))) {
                            PyErr_SetString(PyExc_TypeError, "value must be a byte string or a Unicode string with ASCII code points only");
                            PyMem_Free(forms);
                            curl_formfree(post);
                            Py_XDECREF(ref_params);
                            return NULL;
                        }

                        val = PyLong_AsLong(PyTuple_GET_ITEM(t, j));
                        if (val != CURLFORM_COPYCONTENTS &&
                            val != CURLFORM_FILE &&
                            val != CURLFORM_FILENAME &&
                            val != CURLFORM_CONTENTTYPE &&
                            val != CURLFORM_BUFFER &&
                            val != CURLFORM_BUFFERPTR)
                        {
                            PyErr_SetString(PyExc_TypeError, "unsupported option");
                            PyMem_Free(forms);
                            curl_formfree(post);
                            Py_XDECREF(ref_params);
                            return NULL;
                        }
                        if (PyText_AsStringAndSize(PyTuple_GET_ITEM(t, j+1), &ostr, &olen, &oencoded_obj)) {
                            /* exception should be already set */
                            PyMem_Free(forms);
                            curl_formfree(post);
                            Py_XDECREF(ref_params);
                            return NULL;
                        }
                        forms[k].option = val;
                        forms[k].value = ostr;
                        ++k;
                        if (val == CURLFORM_COPYCONTENTS) {
                            /* Contents can contain \0 bytes so we specify the length */
                            forms[k].option = CURLFORM_CONTENTSLENGTH;
                            forms[k].value = (const char *)olen;
                            ++k;
                        }
                        else if (val == CURLFORM_BUFFERPTR) {
                            PyObject *obj = PyTuple_GET_ITEM(t, j+1);

                            ref_params = PyList_New((Py_ssize_t)0);
                            if (ref_params == NULL) {
                                PyText_EncodedDecref(oencoded_obj);
                                PyMem_Free(forms);
                                curl_formfree(post);
                                return NULL;
                            }
                            
                            /* Ensure that the buffer remains alive until curl_easy_cleanup() */
                            if (PyList_Append(ref_params, obj) != 0) {
                                PyText_EncodedDecref(oencoded_obj);
                                PyMem_Free(forms);
                                curl_formfree(post);
                                Py_DECREF(ref_params);
                                return NULL;
                            }

                            /* As with CURLFORM_COPYCONTENTS, specify the length. */
                            forms[k].option = CURLFORM_BUFFERLENGTH;
                            forms[k].value = (const char *)olen;
                            ++k;
                        }
                    }
                    forms[k].option = CURLFORM_END;
                    res = curl_formadd(&post, &last,
                                       CURLFORM_COPYNAME, nstr,
                                       CURLFORM_NAMELENGTH, (long) nlen,
                                       CURLFORM_ARRAY, forms,
                                       CURLFORM_END);
                    PyText_EncodedDecref(oencoded_obj);
                    PyMem_Free(forms);
                    if (res != CURLE_OK) {
                        curl_formfree(post);
                        Py_XDECREF(ref_params);
                        CURLERROR_RETVAL();
                    }
                } else {
                    /* Some other type was given, ignore */
                    PyText_EncodedDecref(nencoded_obj);
                    curl_formfree(post);
                    Py_XDECREF(ref_params);
                    PyErr_SetString(PyExc_TypeError, "unsupported second type in tuple");
                    return NULL;
                }
                PyText_EncodedDecref(nencoded_obj);
            }
            res = curl_easy_setopt(self->handle, CURLOPT_HTTPPOST, post);
            /* Check for errors */
            if (res != CURLE_OK) {
                curl_formfree(post);
                Py_XDECREF(ref_params);
                CURLERROR_RETVAL();
            }
            /* Finally, free previously allocated httppost, ZAP any
             * buffer references, and update */
            curl_formfree(self->httppost);
            util_curl_xdecref(self, PYCURL_MEMGROUP_HTTPPOST, self->handle);
            self->httppost = post;

            /* The previous list of INCed references was ZAPed above; save
             * the new one so that we can clean it up on the next
             * self->httppost free. */
            self->httppost_ref_list = ref_params;

            Py_RETURN_NONE;
        }

        /* Just to be sure we do not bug off here */
        assert(old_slist != NULL && slist == NULL);

        /* Handle regular list operations on the other options */
        for (i = 0; i < len; i++) {
            PyObject *listitem = PyList_GetItem(obj, i);
            struct curl_slist *nlist;
            char *str;
            PyObject *sencoded_obj;

            if (!PyText_Check(listitem)) {
                curl_slist_free_all(slist);
                PyErr_SetString(PyExc_TypeError, "list items must be byte strings or Unicode strings with ASCII code points only");
                return NULL;
            }
            /* INFO: curl_slist_append() internally does strdup() the data, so
             * no embedded NUL characters allowed here. */
            str = PyText_AsString_NoNUL(listitem, &sencoded_obj);
            if (str == NULL) {
                curl_slist_free_all(slist);
                return NULL;
            }
            nlist = curl_slist_append(slist, str);
            PyText_EncodedDecref(sencoded_obj);
            if (nlist == NULL || nlist->data == NULL) {
                curl_slist_free_all(slist);
                return PyErr_NoMemory();
            }
            slist = nlist;
        }
        res = curl_easy_setopt(self->handle, (CURLoption)option, slist);
        /* Check for errors */
        if (res != CURLE_OK) {
            curl_slist_free_all(slist);
            CURLERROR_RETVAL();
        }
        /* Finally, free previously allocated list and update */
        curl_slist_free_all(*old_slist);
        *old_slist = slist;

        Py_RETURN_NONE;
    }

    /* Handle the case of function objects for callbacks */
    if (PyFunction_Check(obj) || PyCFunction_Check(obj) ||
        PyCallable_Check(obj) || PyMethod_Check(obj)) {
        /* We use function types here to make sure that our callback
         * definitions exactly match the <curl/curl.h> interface.
         */
        const curl_write_callback w_cb = write_callback;
        const curl_write_callback h_cb = header_callback;
        const curl_read_callback r_cb = read_callback;
        const curl_progress_callback pro_cb = progress_callback;
        const curl_debug_callback debug_cb = debug_callback;
        const curl_ioctl_callback ioctl_cb = ioctl_callback;
        const curl_opensocket_callback opensocket_cb = opensocket_callback;
        const curl_seek_callback seek_cb = seek_callback;

        switch(option) {
        case CURLOPT_WRITEFUNCTION:
            if (self->writeheader_fp != NULL) {
                PyErr_SetString(ErrorObject, "cannot combine WRITEFUNCTION with WRITEHEADER option.");
                return NULL;
            }
            Py_INCREF(obj);
            Py_CLEAR(self->writedata_fp);
            Py_CLEAR(self->w_cb);
            self->w_cb = obj;
            curl_easy_setopt(self->handle, CURLOPT_WRITEFUNCTION, w_cb);
            curl_easy_setopt(self->handle, CURLOPT_WRITEDATA, self);
            break;
        case CURLOPT_HEADERFUNCTION:
            Py_INCREF(obj);
            Py_CLEAR(self->h_cb);
            self->h_cb = obj;
            curl_easy_setopt(self->handle, CURLOPT_HEADERFUNCTION, h_cb);
            curl_easy_setopt(self->handle, CURLOPT_WRITEHEADER, self);
            break;
        case CURLOPT_READFUNCTION:
            Py_INCREF(obj);
            Py_CLEAR(self->readdata_fp);
            Py_CLEAR(self->r_cb);
            self->r_cb = obj;
            curl_easy_setopt(self->handle, CURLOPT_READFUNCTION, r_cb);
            curl_easy_setopt(self->handle, CURLOPT_READDATA, self);
            break;
        case CURLOPT_PROGRESSFUNCTION:
            Py_INCREF(obj);
            Py_CLEAR(self->pro_cb);
            self->pro_cb = obj;
            curl_easy_setopt(self->handle, CURLOPT_PROGRESSFUNCTION, pro_cb);
            curl_easy_setopt(self->handle, CURLOPT_PROGRESSDATA, self);
            break;
        case CURLOPT_DEBUGFUNCTION:
            Py_INCREF(obj);
            Py_CLEAR(self->debug_cb);
            self->debug_cb = obj;
            curl_easy_setopt(self->handle, CURLOPT_DEBUGFUNCTION, debug_cb);
            curl_easy_setopt(self->handle, CURLOPT_DEBUGDATA, self);
            break;
        case CURLOPT_IOCTLFUNCTION:
            Py_INCREF(obj);
            Py_CLEAR(self->ioctl_cb);
            self->ioctl_cb = obj;
            curl_easy_setopt(self->handle, CURLOPT_IOCTLFUNCTION, ioctl_cb);
            curl_easy_setopt(self->handle, CURLOPT_IOCTLDATA, self);
            break;
        case CURLOPT_OPENSOCKETFUNCTION:
            Py_INCREF(obj);
            Py_CLEAR(self->opensocket_cb);
            self->opensocket_cb = obj;
            curl_easy_setopt(self->handle, CURLOPT_OPENSOCKETFUNCTION, opensocket_cb);
            curl_easy_setopt(self->handle, CURLOPT_OPENSOCKETDATA, self);
            break;
        case CURLOPT_SEEKFUNCTION:
            Py_INCREF(obj);
            Py_CLEAR(self->seek_cb);
            self->seek_cb = obj;
            curl_easy_setopt(self->handle, CURLOPT_SEEKFUNCTION, seek_cb);
            curl_easy_setopt(self->handle, CURLOPT_SEEKDATA, self);
            break;

        default:
            /* None of the function options were recognized, raise exception */
            PyErr_SetString(PyExc_TypeError, "functions are not supported for this option");
            return NULL;
        }
        Py_RETURN_NONE;
    }
    /* handle the SHARE case */
    if (option == CURLOPT_SHARE) {
        CurlShareObject *share;

        if (self->share == NULL && (obj == NULL || obj == Py_None))
            Py_RETURN_NONE;

        if (self->share) {
            if (obj != Py_None) {
                PyErr_SetString(ErrorObject, "Curl object already sharing. Unshare first.");
                return NULL;
            }
            else {
                share = self->share;
                res = curl_easy_setopt(self->handle, CURLOPT_SHARE, NULL);
                if (res != CURLE_OK) {
                    CURLERROR_RETVAL();
                }
                self->share = NULL;
                Py_DECREF(share);
                Py_RETURN_NONE;
            }
        }
        if (Py_TYPE(obj) != p_CurlShare_Type) {
            PyErr_SetString(PyExc_TypeError, "invalid arguments to setopt");
            return NULL;
        }
        share = (CurlShareObject*)obj;
        res = curl_easy_setopt(self->handle, CURLOPT_SHARE, share->share_handle);
        if (res != CURLE_OK) {
            CURLERROR_RETVAL();
        }
        self->share = share;
        Py_INCREF(share);
        Py_RETURN_NONE;
    }

    /*
    Handle the case of file-like objects for Python 3.
    
    Given an object with a write method, we will call the write method
    from the appropriate callback.
    
    Files in Python 3 are no longer FILE * instances and therefore cannot
    be directly given to curl.
    
    For consistency, ability to use any file-like object is also available
    on Python 2.
    */
    if (option == CURLOPT_READDATA ||
        option == CURLOPT_WRITEDATA ||
        option == CURLOPT_WRITEHEADER)
    {
        const char *method_name;
        PyObject *method;
        
        if (option == CURLOPT_READDATA) {
            method_name = "read";
        } else {
            method_name = "write";
        }
        method = PyObject_GetAttrString(obj, method_name);
        if (method) {
            PyObject *arglist;
            PyObject *rv;
            
            switch (option) {
                case CURLOPT_READDATA:
                    option = CURLOPT_READFUNCTION;
                    break;
                case CURLOPT_WRITEDATA:
                    option = CURLOPT_WRITEFUNCTION;
                    break;
                case CURLOPT_WRITEHEADER:
                    if (self->w_cb != NULL) {
                        PyErr_SetString(ErrorObject, "cannot combine WRITEHEADER with WRITEFUNCTION.");
                        Py_DECREF(method);
                        return NULL;
                    }
                    option = CURLOPT_HEADERFUNCTION;
                    break;
                default:
                    PyErr_SetString(PyExc_TypeError, "objects are not supported for this option");
                    Py_DECREF(method);
                    return NULL;
            }
            
            arglist = Py_BuildValue("(iO)", option, method);
            /* reference is now in arglist */
            Py_DECREF(method);
            if (arglist == NULL) {
                return NULL;
            }
            rv = do_curl_setopt(self, arglist);
            Py_DECREF(arglist);
            return rv;
        } else {
            PyErr_SetString(ErrorObject, "object given without a write method");
            return NULL;
        }
    }

    /* Failed to match any of the function signatures -- return error */
error:
    PyErr_SetString(PyExc_TypeError, "invalid arguments to setopt");
    return NULL;
}