static void dlg_update_routeset(pjsip_dialog *dlg, const pjsip_rx_data *rdata)
{
    const pjsip_hdr *hdr, *end_hdr;
    //pj_int32_t msg_cseq;
    const pjsip_msg *msg;
    const pjsip_method update = { PJSIP_OTHER_METHOD, {"UPDATE", 6}};

    msg = rdata->msg_info.msg;
    //msg_cseq = rdata->msg_info.cseq->cseq;

    /* Ignore if route set has been frozen */
    if (dlg->route_set_frozen)
	return;

    /* Ignore if the message is an UPDATE response (see ticket #1781) */
    if (pjsip_method_cmp(&rdata->msg_info.cseq->method, &update) == 0)
	return;

    /* Only update route set if this message belongs to the same
     * transaction as the initial transaction that establishes dialog.
     */
    if (dlg->role == PJSIP_ROLE_UAC) {

	/* Ignore subsequent request from remote */
	if (msg->type != PJSIP_RESPONSE_MSG)
	    return;

	/* Ignore subsequent responses with higher CSeq than initial CSeq.
	 * Unfortunately this would be broken when the first request is
	 * challenged!
	 */
	//if (msg_cseq != dlg->local.first_cseq)
	//    return;

    } else {

	/* For callee dialog, route set should have been set by initial
	 * request and it will have been rejected by dlg->route_set_frozen
	 * check above.
	 */
	pj_assert(!"Should not happen");

    }

    /* Based on the checks above, we should only get response message here */
    pj_assert(msg->type == PJSIP_RESPONSE_MSG);

    /* Ignore if this is not 1xx or 2xx response */
    if (msg->line.status.code >= 300)
	return;

    /* Reset route set */
    pj_list_init(&dlg->route_set);

    /* Update route set */
    end_hdr = &msg->hdr;
    for (hdr=msg->hdr.prev; hdr!=end_hdr; hdr=hdr->prev) {
	if (hdr->type == PJSIP_H_RECORD_ROUTE) {
	    pjsip_route_hdr *r;
	    r = (pjsip_route_hdr*) pjsip_hdr_clone(dlg->pool, hdr);
	    pjsip_routing_hdr_set_route(r);
	    pj_list_push_back(&dlg->route_set, r);
	}
    }

    PJ_LOG(5,(dlg->obj_name, "Route-set updated"));

    /* Freeze the route set only when the route set comes in 2xx response.
     * If it is in 1xx response, prepare to recompute the route set when
     * the 2xx response comes in.
     *
     * There is a debate whether route set should be frozen when the dialog
     * is established with reliable provisional response, but I think
     * it is safer to not freeze the route set (thus recompute the route set
     * upon receiving 2xx response). Also RFC 3261 says so in 13.2.2.4.
     *
     * The pjsip_method_creates_dialog() check protects from wrongly
     * freezing the route set upon receiving 200/OK response for PRACK.
     */
    if (pjsip_method_creates_dialog(&rdata->msg_info.cseq->method) &&
	PJSIP_IS_STATUS_IN_CLASS(msg->line.status.code, 200))
    {
	dlg->route_set_frozen = PJ_TRUE;
	PJ_LOG(5,(dlg->obj_name, "Route-set frozen"));
    }
}