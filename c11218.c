PJ_DEF(pj_status_t) pjsip_ua_register_dlg( pjsip_user_agent *ua,
					   pjsip_dialog *dlg )
{
    /* Sanity check. */
    PJ_ASSERT_RETURN(ua && dlg, PJ_EINVAL);

    /* For all dialogs, local tag (inc hash) must has been initialized. */
    PJ_ASSERT_RETURN(dlg->local.info && dlg->local.info->tag.slen &&
		     dlg->local.tag_hval != 0, PJ_EBUG);

    /* For UAS dialog, remote tag (inc hash) must have been initialized. */
    //PJ_ASSERT_RETURN(dlg->role==PJSIP_ROLE_UAC ||
    //		     (dlg->role==PJSIP_ROLE_UAS && dlg->remote.info->tag.slen
    //		      && dlg->remote.tag_hval != 0), PJ_EBUG);

    /* Lock the user agent. */
    pj_mutex_lock(mod_ua.mutex);

    /* For UAC, check if there is existing dialog in the same set. */
    if (dlg->role == PJSIP_ROLE_UAC) {
	struct dlg_set *dlg_set;

	dlg_set = (struct dlg_set*)
		  pj_hash_get_lower( mod_ua.dlg_table,
                                     dlg->local.info->tag.ptr, 
			             (unsigned)dlg->local.info->tag.slen,
			             &dlg->local.tag_hval);

	if (dlg_set) {
	    /* This is NOT the first dialog in the dialog set. 
	     * Just add this dialog in the list.
	     */
	    pj_assert(dlg_set->dlg_list.next != (void*)&dlg_set->dlg_list);
	    pj_list_push_back(&dlg_set->dlg_list, dlg);

	    dlg->dlg_set = dlg_set;

	} else {
	    /* This is the first dialog in the dialog set. 
	     * Create the dialog set and add this dialog to it.
	     */
	    dlg_set = alloc_dlgset_node();
	    pj_list_init(&dlg_set->dlg_list);
	    pj_list_push_back(&dlg_set->dlg_list, dlg);

	    dlg->dlg_set = dlg_set;

	    /* Register the dialog set in the hash table. */
	    pj_hash_set_np_lower(mod_ua.dlg_table, 
			         dlg->local.info->tag.ptr,
                                 (unsigned)dlg->local.info->tag.slen,
			         dlg->local.tag_hval, dlg_set->ht_entry,
                                 dlg_set);
	}

    } else {
	/* For UAS, create the dialog set with a single dialog as member. */
	struct dlg_set *dlg_set;

	dlg_set = alloc_dlgset_node();
	pj_list_init(&dlg_set->dlg_list);
	pj_list_push_back(&dlg_set->dlg_list, dlg);

	dlg->dlg_set = dlg_set;

	pj_hash_set_np_lower(mod_ua.dlg_table, 
		             dlg->local.info->tag.ptr,
                             (unsigned)dlg->local.info->tag.slen,
		             dlg->local.tag_hval, dlg_set->ht_entry, dlg_set);
    }

    /* Unlock user agent. */
    pj_mutex_unlock(mod_ua.mutex);

    /* Done. */
    return PJ_SUCCESS;
}