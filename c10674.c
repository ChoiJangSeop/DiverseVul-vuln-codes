static pj_bool_t on_accept_complete2(pj_ssl_sock_t *ssock,
				     pj_ssl_sock_t *new_ssock,
				     const pj_sockaddr_t *src_addr,
				     int src_addr_len, 
				     pj_status_t accept_status)
{    
    struct tls_listener *listener;
    struct tls_transport *tls;
    pj_ssl_sock_info ssl_info;
    char addr[PJ_INET6_ADDRSTRLEN+10];
    pjsip_tp_state_callback state_cb;
    pj_sockaddr tmp_src_addr;
    pj_bool_t is_shutdown;
    pj_status_t status;
    char addr_buf[PJ_INET6_ADDRSTRLEN+10];        

    PJ_UNUSED_ARG(src_addr_len);

    listener = (struct tls_listener*) pj_ssl_sock_get_user_data(ssock);

    if (accept_status != PJ_SUCCESS) {
	if (listener && listener->tls_setting.on_accept_fail_cb) {
	    pjsip_tls_on_accept_fail_param param;
	    pj_ssl_sock_info ssi;

	    pj_bzero(&param, sizeof(param));
	    param.status = accept_status;
	    param.local_addr = &listener->factory.local_addr;
	    param.remote_addr = src_addr;
	    if (new_ssock &&
		pj_ssl_sock_get_info(new_ssock, &ssi) == PJ_SUCCESS)
	    {
		param.last_native_err = ssi.last_native_err;
	    }

	    (*listener->tls_setting.on_accept_fail_cb) (&param);
	}

	return PJ_FALSE;
    }

    PJ_ASSERT_RETURN(new_ssock, PJ_TRUE);

    if (!listener->is_registered) {
	if (listener->tls_setting.on_accept_fail_cb) {
	    pjsip_tls_on_accept_fail_param param;
	    pj_bzero(&param, sizeof(param));
	    param.status = PJSIP_TLS_EACCEPT;
	    param.local_addr = &listener->factory.local_addr;
	    param.remote_addr = src_addr;
	    (*listener->tls_setting.on_accept_fail_cb) (&param);
	}
	return PJ_FALSE;
    }	

    PJ_LOG(4,(listener->factory.obj_name, 
	      "TLS listener %s: got incoming TLS connection "
	      "from %s, sock=%d",
	      pj_addr_str_print(&listener->factory.addr_name.host, 
				listener->factory.addr_name.port, addr_buf, 
				sizeof(addr_buf), 1),
	      pj_sockaddr_print(src_addr, addr, sizeof(addr), 3),
	      new_ssock));

    /* Retrieve SSL socket info, close the socket if this is failed
     * as the SSL socket info availability is rather critical here.
     */
    status = pj_ssl_sock_get_info(new_ssock, &ssl_info);
    if (status != PJ_SUCCESS) {
	pj_ssl_sock_close(new_ssock);

	if (listener->tls_setting.on_accept_fail_cb) {
	    pjsip_tls_on_accept_fail_param param;
	    pj_bzero(&param, sizeof(param));
	    param.status = status;
	    param.local_addr = &listener->factory.local_addr;
	    param.remote_addr = src_addr;
	    (*listener->tls_setting.on_accept_fail_cb) (&param);
	}
	return PJ_TRUE;
    }

    /* Copy to larger buffer, just in case */
    pj_bzero(&tmp_src_addr, sizeof(tmp_src_addr));
    pj_sockaddr_cp(&tmp_src_addr, src_addr);

    /* 
     * Incoming connection!
     * Create TLS transport for the new socket.
     */
    status = tls_create( listener, NULL, new_ssock, PJ_TRUE,
			 &ssl_info.local_addr, &tmp_src_addr, NULL,
			 ssl_info.grp_lock, &tls);
    
    if (status != PJ_SUCCESS) {
	if (listener->tls_setting.on_accept_fail_cb) {
	    pjsip_tls_on_accept_fail_param param;
	    pj_bzero(&param, sizeof(param));
	    param.status = status;
	    param.local_addr = &listener->factory.local_addr;
	    param.remote_addr = src_addr;
	    (*listener->tls_setting.on_accept_fail_cb) (&param);
	}
	return PJ_TRUE;
    }

    /* Set the "pending" SSL socket user data */
    pj_ssl_sock_set_user_data(new_ssock, tls);

    /* Prevent immediate transport destroy as application may access it 
     * (getting info, etc) in transport state notification callback.
     */
    pjsip_transport_add_ref(&tls->base);

    /* If there is verification error and verification is mandatory, shutdown
     * and destroy the transport.
     */
    if (ssl_info.verify_status && listener->tls_setting.verify_client) {
	if (tls->close_reason == PJ_SUCCESS) 
	    tls->close_reason = PJSIP_TLS_ECERTVERIF;
	pjsip_transport_shutdown(&tls->base);
    }
    /* Notify transport state to application */
    state_cb = pjsip_tpmgr_get_state_cb(tls->base.tpmgr);
    if (state_cb) {
	pjsip_transport_state_info state_info;
	pjsip_tls_state_info tls_info;
	pjsip_transport_state tp_state;

	/* Init transport state info */
	pj_bzero(&tls_info, sizeof(tls_info));
	pj_bzero(&state_info, sizeof(state_info));
	tls_info.ssl_sock_info = &ssl_info;
	state_info.ext_info = &tls_info;

	/* Set transport state based on verification status */
	if (ssl_info.verify_status && listener->tls_setting.verify_client)
	{
	    tp_state = PJSIP_TP_STATE_DISCONNECTED;
	    state_info.status = PJSIP_TLS_ECERTVERIF;
	} else {
	    tp_state = PJSIP_TP_STATE_CONNECTED;
	    state_info.status = PJ_SUCCESS;
	}

	(*state_cb)(&tls->base, tp_state, &state_info);
    }

    /* Release transport reference. If transport is shutting down, it may
     * get destroyed here.
     */
    is_shutdown = tls->base.is_shutdown;
    pjsip_transport_dec_ref(&tls->base);
    if (is_shutdown)
	return PJ_TRUE;


    status = tls_start_read(tls);
    if (status != PJ_SUCCESS) {
	PJ_LOG(3,(tls->base.obj_name, "New transport cancelled"));
	tls_init_shutdown(tls, status);
	tls_destroy(&tls->base, status);
    } else {
	/* Start keep-alive timer */
	if (pjsip_cfg()->tls.keep_alive_interval) {
	    pj_time_val delay = {0};	    
	    delay.sec = pjsip_cfg()->tls.keep_alive_interval;
	    pjsip_endpt_schedule_timer(listener->endpt, 
				       &tls->ka_timer, 
				       &delay);
	    tls->ka_timer.id = PJ_TRUE;
	    pj_gettimeofday(&tls->last_activity);
	}
    }

    return PJ_TRUE;
}