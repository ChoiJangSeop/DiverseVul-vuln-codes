PJ_DEF(pj_status_t) pjsip_endpt_send_request_stateless(pjsip_endpoint *endpt, 
				   pjsip_tx_data *tdata,
				   void *token,
				   pjsip_send_callback cb)
{
    pjsip_host_info dest_info;
    pjsip_send_state *stateless_data;
    pj_status_t status;

    PJ_ASSERT_RETURN(endpt && tdata, PJ_EINVAL);

    /* Get destination name to contact. */
    status = pjsip_process_route_set(tdata, &dest_info);
    if (status != PJ_SUCCESS)
	return status;

    /* Keep stateless data. */
    stateless_data = PJ_POOL_ZALLOC_T(tdata->pool, pjsip_send_state);
    stateless_data->token = token;
    stateless_data->endpt = endpt;
    stateless_data->tdata = tdata;
    stateless_data->app_cb = cb;

    /* If destination info has not been initialized (this applies for most
     * all requests except CANCEL), resolve destination host. The processing
     * then resumed when the resolving callback is called. For CANCEL, the
     * destination info must have been copied from the original INVITE so
     * proceed to sending the request directly.
     */
    if (tdata->dest_info.addr.count == 0) {
	/* Copy the destination host name to TX data */
	pj_strdup(tdata->pool, &tdata->dest_info.name, &dest_info.addr.host);

	pjsip_endpt_resolve( endpt, tdata->pool, &dest_info, stateless_data,
			     &stateless_send_resolver_callback);
    } else {
	PJ_LOG(5,(THIS_FILE, "%s: skipping target resolution because "
	                     "address is already set",
			     pjsip_tx_data_get_info(tdata)));
	stateless_send_resolver_callback(PJ_SUCCESS, stateless_data,
					 &tdata->dest_info.addr);
    }
    return PJ_SUCCESS;
}