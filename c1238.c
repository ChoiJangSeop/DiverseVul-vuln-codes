acl_prefetch_http(struct proxy *px, struct session *s, void *l7, unsigned int opt,
                  const struct arg *args, struct sample *smp, int req_vol)
{
	struct http_txn *txn = l7;
	struct http_msg *msg = &txn->req;

	/* Note: hdr_idx.v cannot be NULL in this ACL because the ACL is tagged
	 * as a layer7 ACL, which involves automatic allocation of hdr_idx.
	 */

	if (unlikely(!s || !txn))
		return 0;

	/* Check for a dependency on a request */
	smp->type = SMP_T_BOOL;

	if ((opt & SMP_OPT_DIR) == SMP_OPT_DIR_REQ) {
		if (unlikely(!s->req))
			return 0;

		if (unlikely(txn->req.msg_state < HTTP_MSG_BODY)) {
			if ((msg->msg_state == HTTP_MSG_ERROR) ||
			    buffer_full(s->req->buf, global.tune.maxrewrite)) {
				smp->data.uint = 0;
				return -1;
			}

			/* Try to decode HTTP request */
			if (likely(msg->next < s->req->buf->i))
				http_msg_analyzer(msg, &txn->hdr_idx);

			/* Still no valid request ? */
			if (unlikely(msg->msg_state < HTTP_MSG_BODY)) {
				if ((msg->msg_state == HTTP_MSG_ERROR) ||
				    buffer_full(s->req->buf, global.tune.maxrewrite)) {
					smp->data.uint = 0;
					return -1;
				}
				/* wait for final state */
				smp->flags |= SMP_F_MAY_CHANGE;
				return 0;
			}

			/* OK we just got a valid HTTP request. We have some minor
			 * preparation to perform so that further checks can rely
			 * on HTTP tests.
			 */
			txn->meth = find_http_meth(msg->chn->buf->p, msg->sl.rq.m_l);
			if (txn->meth == HTTP_METH_GET || txn->meth == HTTP_METH_HEAD)
				s->flags |= SN_REDIRECTABLE;

			if (unlikely(msg->sl.rq.v_l == 0) && !http_upgrade_v09_to_v10(txn)) {
				smp->data.uint = 0;
				return -1;
			}
		}

		if (req_vol && txn->rsp.msg_state != HTTP_MSG_RPBEFORE)
			return 0;  /* data might have moved and indexes changed */

		/* otherwise everything's ready for the request */
	}
	else {
		/* Check for a dependency on a response */
		if (txn->rsp.msg_state < HTTP_MSG_BODY)
			return 0;
	}

	/* everything's OK */
	return 1;
}