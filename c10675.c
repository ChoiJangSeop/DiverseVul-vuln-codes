static pj_bool_t on_handshake_complete(pj_ssl_sock_t *ssock, 
				       pj_status_t status)
{
    /* Cancel handshake timer */
    if (ssock->timer.id == TIMER_HANDSHAKE_TIMEOUT) {
	pj_timer_heap_cancel(ssock->param.timer_heap, &ssock->timer);
	ssock->timer.id = TIMER_NONE;
    }

    /* Update certificates info on successful handshake */
    if (status == PJ_SUCCESS)
	ssl_update_certs_info(ssock);

    /* Accepting */
    if (ssock->is_server) {
	if (status != PJ_SUCCESS) {
	    /* Handshake failed in accepting, destroy our self silently. */

	    char buf[PJ_INET6_ADDRSTRLEN+10];

	    PJ_PERROR(3,(ssock->pool->obj_name, status,
			 "Handshake failed in accepting %s",
			 pj_sockaddr_print(&ssock->rem_addr, buf,
					   sizeof(buf), 3)));

	    if (ssock->param.cb.on_accept_complete2) {
		(*ssock->param.cb.on_accept_complete2) 
		      (ssock->parent, ssock, (pj_sockaddr_t*)&ssock->rem_addr, 
		      pj_sockaddr_get_len((pj_sockaddr_t*)&ssock->rem_addr), 
		      status);
	    }

	    /* Originally, this is a workaround for ticket #985. However,
	     * a race condition may occur in multiple worker threads
	     * environment when we are destroying SSL objects while other
	     * threads are still accessing them.
	     * Please see ticket #1930 for more info.
	     */
#if 1 //(defined(PJ_WIN32) && PJ_WIN32!=0)||(defined(PJ_WIN64) && PJ_WIN64!=0)
	    if (ssock->param.timer_heap) {
		pj_time_val interval = {0, PJ_SSL_SOCK_DELAYED_CLOSE_TIMEOUT};
		pj_status_t status1;

		ssock->ssl_state = SSL_STATE_NULL;
		ssl_close_sockets(ssock);

		if (ssock->timer.id != TIMER_NONE) {
		    pj_timer_heap_cancel(ssock->param.timer_heap,
					 &ssock->timer);
		}
		pj_time_val_normalize(&interval);
		status1 = pj_timer_heap_schedule_w_grp_lock(
						 ssock->param.timer_heap, 
						 &ssock->timer,
						 &interval,
						 TIMER_CLOSE,
						 ssock->param.grp_lock);
		if (status1 != PJ_SUCCESS) {
	    	    PJ_PERROR(3,(ssock->pool->obj_name, status,
				 "Failed to schedule a delayed close. "
				 "Race condition may occur."));
		    ssock->timer.id = TIMER_NONE;
		    pj_ssl_sock_close(ssock);
		}
	    } else {
		pj_ssl_sock_close(ssock);
	    }
#else
	    {
		pj_ssl_sock_close(ssock);
	    }
#endif

	    return PJ_FALSE;
	}
	/* Notify application the newly accepted SSL socket */
	if (ssock->param.cb.on_accept_complete2) {
	    pj_bool_t ret;
	    ret = (*ssock->param.cb.on_accept_complete2) 
		    (ssock->parent, ssock, (pj_sockaddr_t*)&ssock->rem_addr, 
		    pj_sockaddr_get_len((pj_sockaddr_t*)&ssock->rem_addr), 
		    status);
	    if (ret == PJ_FALSE)
		return PJ_FALSE;	
	} else if (ssock->param.cb.on_accept_complete) {
	    pj_bool_t ret;
	    ret = (*ssock->param.cb.on_accept_complete)
		      (ssock->parent, ssock, (pj_sockaddr_t*)&ssock->rem_addr,
		       pj_sockaddr_get_len((pj_sockaddr_t*)&ssock->rem_addr));
	    if (ret == PJ_FALSE)
		return PJ_FALSE;
	}
    }

    /* Connecting */
    else {
	/* On failure, reset SSL socket state first, as app may try to 
	 * reconnect in the callback.
	 */
	if (status != PJ_SUCCESS) {
	    /* Server disconnected us, possibly due to SSL nego failure */
	    ssl_reset_sock_state(ssock);
	}
	if (ssock->param.cb.on_connect_complete) {
	    pj_bool_t ret;
	    ret = (*ssock->param.cb.on_connect_complete)(ssock, status);
	    if (ret == PJ_FALSE)
		return PJ_FALSE;
	}
    }

    return PJ_TRUE;
}