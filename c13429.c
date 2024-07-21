agoo_ws_create_req(agooCon c, long mlen) {
    uint8_t	op = 0x0F & *c->buf;

    if (NULL == (c->req = agoo_req_create(mlen))) {
	agoo_log_cat(&agoo_error_cat, "Out of memory attempting to allocate request.");
	return true;
    }
    if (NULL == c->up || agoo_server.ctx_nil_value == c->up->ctx) {
	return true;
    }
    memset(c->req, 0, sizeof(struct _agooReq));
    if ((long)c->bcnt <= mlen) {
	memcpy(c->req->msg, c->buf, c->bcnt);
	if ((long)c->bcnt < mlen) {
	    memset(c->req->msg + c->bcnt, 0, mlen - c->bcnt);
	}
    } else {
	memcpy(c->req->msg, c->buf, mlen);
    }
    c->req->msg[mlen] = '\0';
    c->req->mlen = mlen;
    c->req->method = (AGOO_WS_OP_BIN == op) ? AGOO_ON_BIN : AGOO_ON_MSG;
    c->req->upgrade = AGOO_UP_NONE;
    c->req->up = c->up;
    c->req->res = NULL;
    if (c->up->on_msg) {
	c->req->hook = agoo_hook_create(AGOO_NONE, NULL, c->up->ctx, PUSH_HOOK, &agoo_server.eval_queue);
    }
    return false;
}