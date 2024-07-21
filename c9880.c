void esp_request_cancelled(SCSIRequest *req)
{
    ESPState *s = req->hba_private;

    if (req == s->current_req) {
        scsi_req_unref(s->current_req);
        s->current_req = NULL;
        s->current_dev = NULL;
    }
}