static void mptsas_free_request(MPTSASRequest *req)
{
    MPTSASState *s = req->dev;

    if (req->sreq != NULL) {
        req->sreq->hba_private = NULL;
        scsi_req_unref(req->sreq);
        req->sreq = NULL;
        QTAILQ_REMOVE(&s->pending, req, next);
    }
    qemu_sglist_destroy(&req->qsg);
    g_free(req);
}