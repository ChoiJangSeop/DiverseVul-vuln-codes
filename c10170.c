PJ_DEF(pj_status_t) pjsip_tpmgr_acquire_transport2(pjsip_tpmgr *mgr,
						   pjsip_transport_type_e type,
						   const pj_sockaddr_t *remote,
						   int addr_len,
						   const pjsip_tpselector *sel,
						   pjsip_tx_data *tdata,
						   pjsip_transport **tp)
{
    pjsip_tpfactory *factory;
    pj_status_t status;

    TRACE_((THIS_FILE,"Acquiring transport type=%s, sel=%s remote=%s:%d",
		       pjsip_transport_get_type_name(type),
		       print_tpsel_info(sel),
		       addr_string(remote),
		       pj_sockaddr_get_port(remote)));

    pj_lock_acquire(mgr->lock);

    /* If transport is specified, then just use it if it is suitable
     * for the destination.
     */
    if (sel && sel->type == PJSIP_TPSELECTOR_TRANSPORT &&
	sel->u.transport) 
    {
	pjsip_transport *seltp = sel->u.transport;

	/* See if the transport is (not) suitable */
	if (seltp->key.type != type) {
	    pj_lock_release(mgr->lock);
	    TRACE_((THIS_FILE, "Transport type in tpsel not matched"));
	    return PJSIP_ETPNOTSUITABLE;
	}

	/* Make sure the transport is not being destroyed */
	if (seltp->is_destroying) {
	    pj_lock_release(mgr->lock);
	    TRACE_((THIS_FILE,"Transport to be acquired is being destroyed"));
	    return PJ_ENOTFOUND;
	}

	/* We could also verify that the destination address is reachable
	 * from this transport (i.e. both are equal), but if application
	 * has requested a specific transport to be used, assume that
	 * it knows what to do.
	 *
	 * In other words, I don't think destination verification is a good
	 * idea for now.
	 */

	/* Transport looks to be suitable to use, so just use it. */
	pjsip_transport_add_ref(seltp);
	pj_lock_release(mgr->lock);
	*tp = seltp;

	TRACE_((THIS_FILE, "Transport %s acquired", seltp->obj_name));
	return PJ_SUCCESS;

    } else {

	/*
	 * This is the "normal" flow, where application doesn't specify
	 * specific transport to be used to send message to.
	 * In this case, lookup the transport from the hash table.
	 */
	pjsip_transport_key key;
	int key_len;
	pjsip_transport *tp_ref = NULL;
	transport *tp_entry = NULL;


	/* If listener is specified, verify that the listener type matches
	 * the destination type.
	 */
	if (sel && sel->type == PJSIP_TPSELECTOR_LISTENER && sel->u.listener)
	{
	    if (sel->u.listener->type != type) {
		pj_lock_release(mgr->lock);
		TRACE_((THIS_FILE, "Listener type in tpsel not matched"));
		return PJSIP_ETPNOTSUITABLE;
	    }
	}

	if (!sel || sel->disable_connection_reuse == PJ_FALSE) {
	    pj_bzero(&key, sizeof(key));
	    key_len = sizeof(key.type) + addr_len;

	    /* First try to get exact destination. */
	    key.type = type;
	    pj_memcpy(&key.rem_addr, remote, addr_len);

	    tp_entry = (transport *)pj_hash_get(mgr->table, &key, key_len,
						NULL);
	    if (tp_entry) {
		transport *tp_iter = tp_entry;
		do {
		    /* Don't use transport being shutdown/destroyed */
		    if (!tp_iter->tp->is_shutdown &&
			!tp_iter->tp->is_destroying)
		    {
			if (sel && sel->type == PJSIP_TPSELECTOR_LISTENER &&
			    sel->u.listener)
			{
			    /* Match listener if selector is set */
			    if (tp_iter->tp->factory == sel->u.listener) {
				tp_ref = tp_iter->tp;
				break;
			    }
			} else {
			    tp_ref = tp_iter->tp;
			    break;
			}
		    }
		    tp_iter = tp_iter->next;
		} while (tp_iter != tp_entry);
	    }
	}

	if (tp_ref == NULL &&
	    (!sel || sel->disable_connection_reuse == PJ_FALSE))
	{
	    unsigned flag = pjsip_transport_get_flag_from_type(type);
	    const pj_sockaddr *remote_addr = (const pj_sockaddr*)remote;


	    /* Ignore address for loop transports. */
	    if (type == PJSIP_TRANSPORT_LOOP ||
		type == PJSIP_TRANSPORT_LOOP_DGRAM)
	    {
		pj_sockaddr *addr = &key.rem_addr;

		pj_bzero(addr, addr_len);
		key_len = sizeof(key.type) + addr_len;
		tp_entry = (transport *) pj_hash_get(mgr->table, &key,
						     key_len, NULL);
		if (tp_entry) {
		    tp_ref = tp_entry->tp;
		}
	    }
	    /* For datagram transports, try lookup with zero address.
	     */
	    else if (flag & PJSIP_TRANSPORT_DATAGRAM)
	    {
		pj_sockaddr *addr = &key.rem_addr;

		pj_bzero(addr, addr_len);
		addr->addr.sa_family = remote_addr->addr.sa_family;

		key_len = sizeof(key.type) + addr_len;
		tp_entry = (transport *) pj_hash_get(mgr->table, &key,
						     key_len, NULL);
		if (tp_entry) {
		    tp_ref = tp_entry->tp;
		}
	    }
	}

	/* If transport is found and listener is specified, verify listener */
	else if (sel && sel->type == PJSIP_TPSELECTOR_LISTENER &&
		 sel->u.listener && tp_ref->factory != sel->u.listener)
	{
	    tp_ref = NULL;
	    /* This will cause a new transport to be created which will be a
	     * 'duplicate' of the existing transport (same type & remote addr,
	     * but different factory).
	     */
	    TRACE_((THIS_FILE, "Transport found but from different listener"));
	}

	if (tp_ref!=NULL && !tp_ref->is_shutdown && !tp_ref->is_destroying) {
	    /*
	     * Transport found!
	     */
	    pjsip_transport_add_ref(tp_ref);
	    pj_lock_release(mgr->lock);
	    *tp = tp_ref;

	    TRACE_((THIS_FILE, "Transport %s acquired", tp_ref->obj_name));
	    return PJ_SUCCESS;
	}


	/*
	 * Either transport not found, or we don't want to use the existing
	 * transport (such as in the case of different factory or
	 * if connection reuse is disabled). So we need to create one,
	 * find factory that can create such transport.
	 *
	 * If there's an existing transport, its place in the hash table
	 * will be replaced by this new one. And eventually the existing
	 * transport will still be freed (by application or #1774).
	 */
	if (sel && sel->type == PJSIP_TPSELECTOR_LISTENER && sel->u.listener)
	{
	    /* Application has requested that a specific listener is to
	     * be used.
	     */

	    /* Verify that the listener type matches the destination type */
	    /* Already checked above. */
	    /*
	    if (sel->u.listener->type != type) {
		pj_lock_release(mgr->lock);
		return PJSIP_ETPNOTSUITABLE;
	    }
	    */

	    /* We'll use this listener to create transport */
	    factory = sel->u.listener;

	    /* Verify if listener is still valid */
	    if (!pjsip_tpmgr_is_tpfactory_valid(mgr, factory)) {
		pj_lock_release(mgr->lock);
		PJ_LOG(3,(THIS_FILE, "Specified factory for creating "
				     "transport is not found"));
		return PJ_ENOTFOUND;
	    }

	} else {

	    /* Find factory with type matches the destination type */
	    factory = mgr->factory_list.next;
	    while (factory != &mgr->factory_list) {
		if (factory->type == type)
		    break;
		factory = factory->next;
	    }

	    if (factory == &mgr->factory_list) {
		/* No factory can create the transport! */
		pj_lock_release(mgr->lock);
		TRACE_((THIS_FILE, "No suitable factory was found either"));
		return PJSIP_EUNSUPTRANSPORT;
	    }
	}
    }

    TRACE_((THIS_FILE, "Creating new transport from factory"));

    /* Request factory to create transport. */
    if (factory->create_transport2) {
	status = factory->create_transport2(factory, mgr, mgr->endpt,
					    (const pj_sockaddr*) remote,
					    addr_len, tdata, tp);
    } else {
	status = factory->create_transport(factory, mgr, mgr->endpt,
					   (const pj_sockaddr*) remote,
					   addr_len, tp);
    }
    if (status == PJ_SUCCESS) {
	PJ_ASSERT_ON_FAIL(tp!=NULL,
	    {pj_lock_release(mgr->lock); return PJ_EBUG;});
	pjsip_transport_add_ref(*tp);
	(*tp)->factory = factory;
    }
    pj_lock_release(mgr->lock);
    return status;
}