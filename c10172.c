static pj_status_t dlg_create_request_throw( pjsip_dialog *dlg,
					     const pjsip_method *method,
					     int cseq,
					     pjsip_tx_data **p_tdata )
{
    pjsip_tx_data *tdata;
    pjsip_contact_hdr *contact;
    pjsip_route_hdr *route, *end_list;
    pj_status_t status;

    /* Contact Header field.
     * Contact can only be present in requests that establish dialog (in the
     * core SIP spec, only INVITE).
     */
    if (pjsip_method_creates_dialog(method))
	contact = dlg->local.contact;
    else
	contact = NULL;

    /*
     * Create the request by cloning from the headers in the
     * dialog.
     */
    status = pjsip_endpt_create_request_from_hdr(dlg->endpt,
						 method,
						 dlg->target,
						 dlg->local.info,
						 dlg->remote.info,
						 contact,
						 dlg->call_id,
						 cseq,
						 NULL,
						 &tdata);
    if (status != PJ_SUCCESS)
	return status;

    /* Put this dialog in tdata's mod_data */
    tdata->mod_data[dlg->ua->id] = dlg;

    /* Just copy dialog route-set to Route header.
     * The transaction will do the processing as specified in Section 12.2.1
     * of RFC 3261 in function tsx_process_route() in sip_transaction.c.
     */
    route = dlg->route_set.next;
    end_list = &dlg->route_set;
    for (; route != end_list; route = route->next ) {
	pjsip_route_hdr *r;
	r = (pjsip_route_hdr*) pjsip_hdr_shallow_clone( tdata->pool, route );
	pjsip_routing_hdr_set_route(r);
	pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)r);
    }

    /* Copy authorization headers, if request is not ACK or CANCEL. */
    if (method->id != PJSIP_ACK_METHOD && method->id != PJSIP_CANCEL_METHOD) {
	status = pjsip_auth_clt_init_req( &dlg->auth_sess, tdata );
	if (status != PJ_SUCCESS)
	    return status;
    }

    /* Done. */
    *p_tdata = tdata;

    return PJ_SUCCESS;
}