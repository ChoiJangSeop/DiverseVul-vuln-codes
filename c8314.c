int meth_put(struct transaction_t *txn, void *params)
{
    struct meth_params *pparams = (struct meth_params *) params;
    int ret, r, precond, rights, reqd_rights;
    const char **hdr, *etag;
    struct mime_type_t *mime = NULL;
    struct mailbox *mailbox = NULL;
    struct dav_data *ddata;
    struct index_record oldrecord;
    quota_t qdiffs[QUOTA_NUMRESOURCES] = QUOTA_DIFFS_INITIALIZER;
    time_t lastmod;
    unsigned flags = 0;
    void *davdb = NULL, *obj = NULL;
    struct buf msg_buf = BUF_INITIALIZER;

    if (txn->meth == METH_POST) {
        reqd_rights = DACL_ADDRSRC;
    }
    else {
        /* Response should not be cached */
        txn->flags.cc |= CC_NOCACHE;

        /* Parse the path */
        r = dav_parse_req_target(txn, pparams);
        if (r) {
            switch (r){
            case HTTP_MOVED:
            case HTTP_SERVER_ERROR: return r;
            default: return HTTP_FORBIDDEN;
            }
        }

        /* Make sure method is allowed (only allowed on resources) */
        if (!((txn->req_tgt.allow & ALLOW_WRITE) && txn->req_tgt.resource))
            return HTTP_NOT_ALLOWED;

        reqd_rights = DACL_WRITECONT;

        if (txn->req_tgt.allow & ALLOW_USERDATA) reqd_rights |= DACL_PROPRSRC;
    }

    /* Make sure mailbox type is correct */
    if (txn->req_tgt.mbentry->mbtype != txn->req_tgt.namespace->mboxtype)
        return HTTP_FORBIDDEN;

    /* Make sure Content-Range isn't specified */
    if (spool_getheader(txn->req_hdrs, "Content-Range"))
        return HTTP_BAD_REQUEST;

    /* Check Content-Type */
    mime = pparams->mime_types;
    if ((hdr = spool_getheader(txn->req_hdrs, "Content-Type"))) {
        for (; mime->content_type; mime++) {
            if (is_mediatype(mime->content_type, hdr[0])) break;
        }
        if (!mime->content_type) {
            txn->error.precond = pparams->put.supp_data_precond;
            return HTTP_FORBIDDEN;
        }
    }

    /* Check ACL for current user */
    rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry);
    if (!(rights & reqd_rights)) {
        /* DAV:need-privileges */
        txn->error.precond = DAV_NEED_PRIVS;
        txn->error.resource = txn->req_tgt.path;
        txn->error.rights = reqd_rights;
        return HTTP_NO_PRIVS;
    }

    if (txn->req_tgt.mbentry->server) {
        /* Remote mailbox */
        struct backend *be;

        be = proxy_findserver(txn->req_tgt.mbentry->server,
                              &http_protocol, httpd_userid,
                              &backend_cached, NULL, NULL, httpd_in);
        if (!be) return HTTP_UNAVAILABLE;

        return http_pipe_req_resp(be, txn);
    }

    /* Local Mailbox */

    /* Read body */
    txn->req_body.flags |= BODY_DECODE;
    ret = http_read_req_body(txn);
    if (ret) {
        txn->flags.conn = CONN_CLOSE;
        return ret;
    }

    if (rights & DACL_WRITECONT) {
        /* Check if we can append a new message to mailbox */
        qdiffs[QUOTA_STORAGE] = buf_len(&txn->req_body.payload);
        if ((r = append_check(txn->req_tgt.mbentry->name, httpd_authstate,
                              ACL_INSERT, ignorequota ? NULL : qdiffs))) {
            syslog(LOG_ERR, "append_check(%s) failed: %s",
                   txn->req_tgt.mbentry->name, error_message(r));
            txn->error.desc = error_message(r);
            return HTTP_SERVER_ERROR;
        }
    }

    /* Open mailbox for writing */
    r = mailbox_open_iwl(txn->req_tgt.mbentry->name, &mailbox);
    if (r) {
        syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
               txn->req_tgt.mbentry->name, error_message(r));
        txn->error.desc = error_message(r);
        return HTTP_SERVER_ERROR;
    }

    /* Open the DAV DB corresponding to the mailbox */
    davdb = pparams->davdb.open_db(mailbox);

    /* Find message UID for the resource, if exists */
    pparams->davdb.lookup_resource(davdb, txn->req_tgt.mbentry->name,
                                   txn->req_tgt.resource, (void *) &ddata, 0);
    /* XXX  Check errors */

    /* Fetch resource validators */
    r = pparams->get_validators(mailbox, (void *) ddata, httpd_userid,
                                &oldrecord, &etag, &lastmod);
    if (r) {
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    /* Check any preferences */
    flags = get_preferences(txn);

    /* Check any preconditions */
    if (txn->meth == METH_POST) {
        assert(!buf_len(&txn->buf));
        buf_printf(&txn->buf, "%u-%u-%u", mailbox->i.uidvalidity,
                   mailbox->i.last_uid, mailbox->i.exists);
        ret = precond = pparams->check_precond(txn, params, mailbox, NULL,
                                               buf_cstring(&txn->buf),
                                               mailbox->index_mtime);
        buf_reset(&txn->buf);
    }
    else {
        ret = precond = pparams->check_precond(txn, params, mailbox,
                                               (void *) ddata, etag, lastmod);
    }

    switch (precond) {
    case HTTP_OK:
        /* Parse, validate, and store the resource */
        obj = mime->to_object(&txn->req_body.payload);
        ret = pparams->put.proc(txn, obj, mailbox,
                                txn->req_tgt.resource, davdb, flags);
        break;

    case HTTP_PRECOND_FAILED:
        if ((flags & PREFER_REP) && ((rights & DACL_READ) == DACL_READ)) {
            /* Fill in ETag and Last-Modified */
            txn->resp_body.etag = etag;
            txn->resp_body.lastmod = lastmod;

            if (pparams->get) {
                r = pparams->get(txn, mailbox, &oldrecord, (void *) ddata, &obj);
                if (r != HTTP_CONTINUE) flags &= ~PREFER_REP;
            }
            else {
                unsigned offset;
                struct buf buf;

                /* Load message containing the resource */
                mailbox_map_record(mailbox, &oldrecord, &msg_buf);

                /* Resource length doesn't include RFC 5322 header */
                offset = oldrecord.header_size;

                /* Parse existing resource */
                buf_init_ro(&buf, buf_base(&msg_buf) + offset,
                            buf_len(&msg_buf) - offset);
                obj = pparams->mime_types[0].to_object(&buf);
                buf_free(&buf);
            }
        }
        break;

    case HTTP_LOCKED:
        txn->error.precond = DAV_NEED_LOCK_TOKEN;
        txn->error.resource = txn->req_tgt.path;

    default:
        /* We failed a precondition */
        goto done;
    }

    if (txn->req_tgt.allow & ALLOW_PATCH) {
        /* Add Accept-Patch formats to response */
        txn->resp_body.patch = pparams->patch_docs;
    }

    if (flags & PREFER_REP) {
        struct resp_body_t *resp_body = &txn->resp_body;
        const char **hdr;
        struct buf *data;

        if ((hdr = spool_getheader(txn->req_hdrs, "Accept"))) {
            mime = get_accept_type(hdr, pparams->mime_types);
            if (!mime) goto done;
        }

        switch (ret) {
        case HTTP_NO_CONTENT:
            ret = HTTP_OK;

            GCC_FALLTHROUGH

        case HTTP_CREATED:
        case HTTP_PRECOND_FAILED:
            /* Convert into requested MIME type */
            data = mime->from_object(obj);

            /* Fill in Content-Type, Content-Length */
            resp_body->type = mime->content_type;
            resp_body->len = buf_len(data);

            /* Fill in Content-Location */
            resp_body->loc = txn->req_tgt.path;

            /* Fill in Expires and Cache-Control */
            resp_body->maxage = 3600;   /* 1 hr */
            txn->flags.cc = CC_MAXAGE
                | CC_REVALIDATE         /* don't use stale data */
                | CC_NOTRANSFORM;       /* don't alter iCal data */

            /* Output current representation */
            write_body(ret, txn, buf_base(data), buf_len(data));

            buf_destroy(data);
            ret = 0;
            break;

        default:
            /* failure - do nothing */
            break;
        }
    }

  done:
    if (obj && pparams->mime_types[0].free)
        pparams->mime_types[0].free(obj);
    buf_free(&msg_buf);
    if (davdb) pparams->davdb.close_db(davdb);
    mailbox_close(&mailbox);

    return ret;
}