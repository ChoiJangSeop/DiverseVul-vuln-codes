PJ_DEF(pj_status_t) pjsip_endpt_send_response( pjsip_endpoint *endpt,
					       pjsip_response_addr *res_addr,
					       pjsip_tx_data *tdata,
					       void *token,
					       pjsip_send_callback cb)
{
    /* Determine which transports and addresses to send the response,
     * based on Section 18.2.2 of RFC 3261.
     */
    pjsip_send_state *send_state;
    pj_status_t status;

    /* Create structure to keep the sending state. */
    send_state = PJ_POOL_ZALLOC_T(tdata->pool, pjsip_send_state);
    send_state->endpt = endpt;
    send_state->tdata = tdata;
    send_state->token = token;
    send_state->app_cb = cb;

    if (res_addr->transport != NULL) {
	send_state->cur_transport = res_addr->transport;
	pjsip_transport_add_ref(send_state->cur_transport);

	status = pjsip_transport_send( send_state->cur_transport, tdata, 
				       &res_addr->addr,
				       res_addr->addr_len,
				       send_state,
				       &send_response_transport_cb );
	if (status == PJ_SUCCESS) {
	    pj_ssize_t sent = tdata->buf.cur - tdata->buf.start;
	    send_response_transport_cb(send_state, tdata, sent);
	    return PJ_SUCCESS;
	} else if (status == PJ_EPENDING) {
	    /* Callback will be called later. */
	    return PJ_SUCCESS;
	} else {
	    pjsip_transport_dec_ref(send_state->cur_transport);
	    return status;
	}
    } else {
	/* Copy the destination host name to TX data */
	pj_strdup(tdata->pool, &tdata->dest_info.name, 
		  &res_addr->dst_host.addr.host);

	pjsip_endpt_resolve(endpt, tdata->pool, &res_addr->dst_host, 
			    send_state, &send_response_resolver_cb);
	return PJ_SUCCESS;
    }
}