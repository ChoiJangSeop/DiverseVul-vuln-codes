pj_status_t create_uas_dialog( pjsip_user_agent *ua,
			       pjsip_rx_data *rdata,
			       const pj_str_t *contact,
			       pj_bool_t inc_lock,
			       pjsip_dialog **p_dlg)
{
    pj_status_t status;
    pjsip_hdr *pos = NULL;
    pjsip_contact_hdr *contact_hdr;
    pjsip_rr_hdr *rr;
    pjsip_transaction *tsx = NULL;
    pj_str_t tmp;
    enum { TMP_LEN=PJSIP_MAX_URL_SIZE };
    pj_ssize_t len;
    pjsip_dialog *dlg;
    pj_bool_t lock_incremented = PJ_FALSE;

    /* Check arguments. */
    PJ_ASSERT_RETURN(ua && rdata && p_dlg, PJ_EINVAL);

    /* rdata must have request message. */
    PJ_ASSERT_RETURN(rdata->msg_info.msg->type == PJSIP_REQUEST_MSG,
		     PJSIP_ENOTREQUESTMSG);

    /* Request must not have To tag.
     * This should have been checked in the user agent (or application?).
     */
    PJ_ASSERT_RETURN(rdata->msg_info.to->tag.slen == 0, PJ_EINVALIDOP);

    /* The request must be a dialog establishing request. */
    PJ_ASSERT_RETURN(
	pjsip_method_creates_dialog(&rdata->msg_info.msg->line.req.method),
	PJ_EINVALIDOP);

    /* Create dialog instance. */
    status = create_dialog(ua, NULL, &dlg);
    if (status != PJ_SUCCESS)
	return status;

    /* Temprary string for getting the string representation of
     * both local and remote URI.
     */
    tmp.ptr = (char*) pj_pool_alloc(rdata->tp_info.pool, TMP_LEN);

    /* Init local info from the To header. */
    dlg->local.info = (pjsip_fromto_hdr*)
    		      pjsip_hdr_clone(dlg->pool, rdata->msg_info.to);
    pjsip_fromto_hdr_set_from(dlg->local.info);

    /* Generate local tag. */
    pj_create_unique_string(dlg->pool, &dlg->local.info->tag);


    /* Print the local info. */
    len = pjsip_uri_print(PJSIP_URI_IN_FROMTO_HDR,
			  dlg->local.info->uri, tmp.ptr, TMP_LEN);
    if (len < 1) {
	pj_ansi_strcpy(tmp.ptr, "<-error: uri too long->");
	tmp.slen = pj_ansi_strlen(tmp.ptr);
    } else
	tmp.slen = len;

    /* Save the local info. */
    pj_strdup(dlg->pool, &dlg->local.info_str, &tmp);

    /* Calculate hash value of local tag. */
    dlg->local.tag_hval = pj_hash_calc_tolower(0, NULL, &dlg->local.info->tag);


    /* Randomize local cseq */
    dlg->local.first_cseq = pj_rand() & 0x7FFF;
    dlg->local.cseq = dlg->local.first_cseq;

    /* Init local contact. */
    /* TODO:
     *  Section 12.1.1, paragraph about using SIPS URI in Contact.
     *  If the request that initiated the dialog contained a SIPS URI
     *  in the Request-URI or in the top Record-Route header field value,
     *  if there was any, or the Contact header field if there was no
     *  Record-Route header field, the Contact header field in the response
     *  MUST be a SIPS URI.
     */
    if (contact) {
	pj_str_t tmp2;

	pj_strdup_with_null(dlg->pool, &tmp2, contact);
	dlg->local.contact = (pjsip_contact_hdr*)
			     pjsip_parse_hdr(dlg->pool, &HCONTACT, tmp2.ptr,
					     tmp2.slen, NULL);
	if (!dlg->local.contact) {
	    status = PJSIP_EINVALIDURI;
	    goto on_error;
	}

    } else {
	dlg->local.contact = pjsip_contact_hdr_create(dlg->pool);
	dlg->local.contact->uri = dlg->local.info->uri;
    }

    /* Init remote info from the From header. */
    dlg->remote.info = (pjsip_fromto_hdr*)
    		       pjsip_hdr_clone(dlg->pool, rdata->msg_info.from);
    pjsip_fromto_hdr_set_to(dlg->remote.info);

    /* Print the remote info. */
    len = pjsip_uri_print(PJSIP_URI_IN_FROMTO_HDR,
			  dlg->remote.info->uri, tmp.ptr, TMP_LEN);
    if (len < 1) {
	pj_ansi_strcpy(tmp.ptr, "<-error: uri too long->");
	tmp.slen = pj_ansi_strlen(tmp.ptr);
    } else
	tmp.slen = len;

    /* Save the remote info. */
    pj_strdup(dlg->pool, &dlg->remote.info_str, &tmp);


    /* Init remote's contact from Contact header.
     * Iterate the Contact URI until we find sip: or sips: scheme.
     */
    do {
	contact_hdr = (pjsip_contact_hdr*)
		      pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_CONTACT,
				         pos);
	if (contact_hdr) {
	    if (!contact_hdr->uri ||
		(!PJSIP_URI_SCHEME_IS_SIP(contact_hdr->uri) &&
		 !PJSIP_URI_SCHEME_IS_SIPS(contact_hdr->uri)))
	    {
		pos = (pjsip_hdr*)contact_hdr->next;
		if (pos == &rdata->msg_info.msg->hdr)
		    contact_hdr = NULL;
	    } else {
		break;
	    }
	}
    } while (contact_hdr);

    if (!contact_hdr) {
	status = PJSIP_ERRNO_FROM_SIP_STATUS(PJSIP_SC_BAD_REQUEST);
	goto on_error;
    }

    dlg->remote.contact = (pjsip_contact_hdr*)
    			  pjsip_hdr_clone(dlg->pool, (pjsip_hdr*)contact_hdr);

    /* Init remote's CSeq from CSeq header */
    dlg->remote.cseq = dlg->remote.first_cseq = rdata->msg_info.cseq->cseq;

    /* Set initial target to remote's Contact. */
    dlg->target = dlg->remote.contact->uri;

    /* Initial role is UAS */
    dlg->role = PJSIP_ROLE_UAS;

    /* Secure?
     *  RFC 3261 Section 12.1.1:
     *  If the request arrived over TLS, and the Request-URI contained a
     *  SIPS URI, the 'secure' flag is set to TRUE.
     */
    dlg->secure = PJSIP_TRANSPORT_IS_SECURE(rdata->tp_info.transport) &&
		  PJSIP_URI_SCHEME_IS_SIPS(rdata->msg_info.msg->line.req.uri);

    /* Call-ID */
    dlg->call_id = (pjsip_cid_hdr*)
    		   pjsip_hdr_clone(dlg->pool, rdata->msg_info.cid);

    /* Route set.
     *  RFC 3261 Section 12.1.1:
     *  The route set MUST be set to the list of URIs in the Record-Route
     *  header field from the request, taken in order and preserving all URI
     *  parameters. If no Record-Route header field is present in the request,
     * the route set MUST be set to the empty set.
     */
    pj_list_init(&dlg->route_set);
    rr = rdata->msg_info.record_route;
    while (rr != NULL) {
	pjsip_route_hdr *route;

	/* Clone the Record-Route, change the type to Route header. */
	route = (pjsip_route_hdr*) pjsip_hdr_clone(dlg->pool, rr);
	pjsip_routing_hdr_set_route(route);

	/* Add to route set. */
	pj_list_push_back(&dlg->route_set, route);

	/* Find next Record-Route header. */
	rr = rr->next;
	if (rr == (void*)&rdata->msg_info.msg->hdr)
	    break;
	rr = (pjsip_route_hdr*) pjsip_msg_find_hdr(rdata->msg_info.msg,
						   PJSIP_H_RECORD_ROUTE, rr);
    }
    dlg->route_set_frozen = PJ_TRUE;

    /* Increment the dialog's lock since tsx may cause the dialog to be
     * destroyed prematurely (such as in case of transport error).
     */
    if (inc_lock) {
        pjsip_dlg_inc_lock(dlg);
        lock_incremented = PJ_TRUE;
    }

    /* Create UAS transaction for this request. */
    status = pjsip_tsx_create_uas(dlg->ua, rdata, &tsx);
    if (status != PJ_SUCCESS)
	goto on_error;

    /* Associate this dialog to the transaction. */
    tsx->mod_data[dlg->ua->id] = dlg;

    /* Increment tsx counter */
    ++dlg->tsx_count;

    /* Calculate hash value of remote tag. */
    dlg->remote.tag_hval = pj_hash_calc_tolower(0, NULL, &dlg->remote.info->tag);

    /* Update remote capabilities info */
    pjsip_dlg_update_remote_cap(dlg, rdata->msg_info.msg, PJ_TRUE);

    /* Register this dialog to user agent. */
    status = pjsip_ua_register_dlg( ua, dlg );
    if (status != PJ_SUCCESS)
	goto on_error;

    /* Put this dialog in rdata's mod_data */
    rdata->endpt_info.mod_data[ua->id] = dlg;

    PJ_TODO(DIALOG_APP_TIMER);

    /* Feed the first request to the transaction. */
    pjsip_tsx_recv_msg(tsx, rdata);

    /* Done. */
    *p_dlg = dlg;
    PJ_LOG(5,(dlg->obj_name, "UAS dialog created"));
    return PJ_SUCCESS;

on_error:
    if (tsx) {
	pjsip_tsx_terminate(tsx, 500);
	pj_assert(dlg->tsx_count>0);
	--dlg->tsx_count;
    }

    if (lock_incremented) {
        pjsip_dlg_dec_lock(dlg);
    } else {
        destroy_dialog(dlg, PJ_FALSE);
    }

    return status;
}