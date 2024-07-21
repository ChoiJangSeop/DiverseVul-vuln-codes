psutil_net_connections(PyObject *self, PyObject *args) {
    long pid;
    int sd = 0;
    mib2_tcpConnEntry_t tp;
    mib2_udpEntry_t     ude;
#if defined(AF_INET6)
    mib2_tcp6ConnEntry_t tp6;
    mib2_udp6Entry_t     ude6;
#endif
    char buf[512];
    int i, flags, getcode, num_ent, state, ret;
    char lip[INET6_ADDRSTRLEN], rip[INET6_ADDRSTRLEN];
    int lport, rport;
    int processed_pid;
    int databuf_init = 0;
    struct strbuf ctlbuf, databuf;
    struct T_optmgmt_req tor = {0};
    struct T_optmgmt_ack toa = {0};
    struct T_error_ack   tea = {0};
    struct opthdr        mibhdr = {0};

    PyObject *py_retlist = PyList_New(0);
    PyObject *py_tuple = NULL;
    PyObject *py_laddr = NULL;
    PyObject *py_raddr = NULL;

    if (py_retlist == NULL)
        return NULL;
    if (! PyArg_ParseTuple(args, "l", &pid))
        goto error;

    sd = open("/dev/arp", O_RDWR);
    if (sd == -1) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, "/dev/arp");
        goto error;
    }

    ret = ioctl(sd, I_PUSH, "tcp");
    if (ret == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }
    ret = ioctl(sd, I_PUSH, "udp");
    if (ret == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }
    //
    // OK, this mess is basically copied and pasted from nxsensor project
    // which copied and pasted it from netstat source code, mibget()
    // function.  Also see:
    // http://stackoverflow.com/questions/8723598/
    tor.PRIM_type = T_SVR4_OPTMGMT_REQ;
    tor.OPT_offset = sizeof (struct T_optmgmt_req);
    tor.OPT_length = sizeof (struct opthdr);
    tor.MGMT_flags = T_CURRENT;
    mibhdr.level = MIB2_IP;
    mibhdr.name  = 0;

#ifdef NEW_MIB_COMPLIANT
    mibhdr.len   = 1;
#else
    mibhdr.len   = 0;
#endif
    memcpy(buf, &tor, sizeof tor);
    memcpy(buf + tor.OPT_offset, &mibhdr, sizeof mibhdr);

    ctlbuf.buf = buf;
    ctlbuf.len = tor.OPT_offset + tor.OPT_length;
    flags = 0;  // request to be sent in non-priority

    if (putmsg(sd, &ctlbuf, (struct strbuf *)0, flags) == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    ctlbuf.maxlen = sizeof (buf);
    for (;;) {
        flags = 0;
        getcode = getmsg(sd, &ctlbuf, (struct strbuf *)0, &flags);
        memcpy(&toa, buf, sizeof toa);
        memcpy(&tea, buf, sizeof tea);

        if (getcode != MOREDATA ||
                ctlbuf.len < (int)sizeof (struct T_optmgmt_ack) ||
                toa.PRIM_type != T_OPTMGMT_ACK ||
                toa.MGMT_flags != T_SUCCESS)
        {
            break;
        }
        if (ctlbuf.len >= (int)sizeof (struct T_error_ack) &&
                tea.PRIM_type == T_ERROR_ACK)
        {
            PyErr_SetString(PyExc_RuntimeError, "ERROR_ACK");
            goto error;
        }
        if (getcode == 0 &&
                ctlbuf.len >= (int)sizeof (struct T_optmgmt_ack) &&
                toa.PRIM_type == T_OPTMGMT_ACK &&
                toa.MGMT_flags == T_SUCCESS)
        {
            PyErr_SetString(PyExc_RuntimeError, "ERROR_T_OPTMGMT_ACK");
            goto error;
        }

        memset(&mibhdr, 0x0, sizeof(mibhdr));
        memcpy(&mibhdr, buf + toa.OPT_offset, toa.OPT_length);

        databuf.maxlen = mibhdr.len;
        databuf.len = 0;
        databuf.buf = (char *)malloc((int)mibhdr.len);
        if (!databuf.buf) {
            PyErr_NoMemory();
            goto error;
        }
        databuf_init = 1;

        flags = 0;
        getcode = getmsg(sd, (struct strbuf *)0, &databuf, &flags);
        if (getcode < 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            goto error;
        }

        // TCPv4
        if (mibhdr.level == MIB2_TCP && mibhdr.name == MIB2_TCP_13) {
            num_ent = mibhdr.len / sizeof(mib2_tcpConnEntry_t);
            for (i = 0; i < num_ent; i++) {
                memcpy(&tp, databuf.buf + i * sizeof tp, sizeof tp);
#ifdef NEW_MIB_COMPLIANT
                processed_pid = tp.tcpConnCreationProcess;
#else
                processed_pid = 0;
#endif
                if (pid != -1 && processed_pid != pid)
                    continue;
                // construct local/remote addresses
                inet_ntop(AF_INET, &tp.tcpConnLocalAddress, lip, sizeof(lip));
                inet_ntop(AF_INET, &tp.tcpConnRemAddress, rip, sizeof(rip));
                lport = tp.tcpConnLocalPort;
                rport = tp.tcpConnRemPort;

                // contruct python tuple/list
                py_laddr = Py_BuildValue("(si)", lip, lport);
                if (!py_laddr)
                    goto error;
                if (rport != 0)
                    py_raddr = Py_BuildValue("(si)", rip, rport);
                else {
                    py_raddr = Py_BuildValue("()");
                }
                if (!py_raddr)
                    goto error;
                state = tp.tcpConnEntryInfo.ce_state;

                // add item
                py_tuple = Py_BuildValue("(iiiNNiI)", -1, AF_INET, SOCK_STREAM,
                                         py_laddr, py_raddr, state,
                                         processed_pid);
                if (!py_tuple)
                    goto error;
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
            }
        }
#if defined(AF_INET6)
        // TCPv6
        else if (mibhdr.level == MIB2_TCP6 && mibhdr.name == MIB2_TCP6_CONN)
        {
            num_ent = mibhdr.len / sizeof(mib2_tcp6ConnEntry_t);

            for (i = 0; i < num_ent; i++) {
                memcpy(&tp6, databuf.buf + i * sizeof tp6, sizeof tp6);
#ifdef NEW_MIB_COMPLIANT
                processed_pid = tp6.tcp6ConnCreationProcess;
#else
        		processed_pid = 0;
#endif
                if (pid != -1 && processed_pid != pid)
                    continue;
                // construct local/remote addresses
                inet_ntop(AF_INET6, &tp6.tcp6ConnLocalAddress, lip, sizeof(lip));
                inet_ntop(AF_INET6, &tp6.tcp6ConnRemAddress, rip, sizeof(rip));
                lport = tp6.tcp6ConnLocalPort;
                rport = tp6.tcp6ConnRemPort;

                // contruct python tuple/list
                py_laddr = Py_BuildValue("(si)", lip, lport);
                if (!py_laddr)
                    goto error;
                if (rport != 0)
                    py_raddr = Py_BuildValue("(si)", rip, rport);
                else
                    py_raddr = Py_BuildValue("()");
                if (!py_raddr)
                    goto error;
                state = tp6.tcp6ConnEntryInfo.ce_state;

                // add item
                py_tuple = Py_BuildValue("(iiiNNiI)", -1, AF_INET6, SOCK_STREAM,
                                         py_laddr, py_raddr, state, processed_pid);
                if (!py_tuple)
                    goto error;
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
            }
        }
#endif
        // UDPv4
        else if (mibhdr.level == MIB2_UDP || mibhdr.level == MIB2_UDP_ENTRY) {
            num_ent = mibhdr.len / sizeof(mib2_udpEntry_t);
	    assert(num_ent * sizeof(mib2_udpEntry_t) == mibhdr.len);
            for (i = 0; i < num_ent; i++) {
                memcpy(&ude, databuf.buf + i * sizeof ude, sizeof ude);
#ifdef NEW_MIB_COMPLIANT
                processed_pid = ude.udpCreationProcess;
#else
                processed_pid = 0;
#endif
                if (pid != -1 && processed_pid != pid)
                    continue;
                // XXX Very ugly hack! It seems we get here only the first
                // time we bump into a UDPv4 socket.  PID is a very high
                // number (clearly impossible) and the address does not
                // belong to any valid interface.  Not sure what else
                // to do other than skipping.
                if (processed_pid > 131072)
                    continue;
                inet_ntop(AF_INET, &ude.udpLocalAddress, lip, sizeof(lip));
                lport = ude.udpLocalPort;
                py_laddr = Py_BuildValue("(si)", lip, lport);
                if (!py_laddr)
                    goto error;
                py_raddr = Py_BuildValue("()");
                if (!py_raddr)
                    goto error;
                py_tuple = Py_BuildValue("(iiiNNiI)", -1, AF_INET, SOCK_DGRAM,
                                         py_laddr, py_raddr, PSUTIL_CONN_NONE,
                                         processed_pid);
                if (!py_tuple)
                    goto error;
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
            }
        }
#if defined(AF_INET6)
        // UDPv6
        else if (mibhdr.level == MIB2_UDP6 ||
                    mibhdr.level == MIB2_UDP6_ENTRY)
            {
            num_ent = mibhdr.len / sizeof(mib2_udp6Entry_t);
            for (i = 0; i < num_ent; i++) {
                memcpy(&ude6, databuf.buf + i * sizeof ude6, sizeof ude6);
#ifdef NEW_MIB_COMPLIANT
                processed_pid = ude6.udp6CreationProcess;
#else
                processed_pid = 0;
#endif
                if (pid != -1 && processed_pid != pid)
                    continue;
                inet_ntop(AF_INET6, &ude6.udp6LocalAddress, lip, sizeof(lip));
                lport = ude6.udp6LocalPort;
                py_laddr = Py_BuildValue("(si)", lip, lport);
                if (!py_laddr)
                    goto error;
                py_raddr = Py_BuildValue("()");
                if (!py_raddr)
                    goto error;
                py_tuple = Py_BuildValue("(iiiNNiI)", -1, AF_INET6, SOCK_DGRAM,
                                         py_laddr, py_raddr, PSUTIL_CONN_NONE,
                                         processed_pid);
                if (!py_tuple)
                    goto error;
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
            }
        }
#endif
        free(databuf.buf);
    }

    close(sd);
    return py_retlist;

error:
    Py_XDECREF(py_tuple);
    Py_XDECREF(py_laddr);
    Py_XDECREF(py_raddr);
    Py_DECREF(py_retlist);
    if (databuf_init == 1)
        free(databuf.buf);
    if (sd != 0)
        close(sd);
    return NULL;
}