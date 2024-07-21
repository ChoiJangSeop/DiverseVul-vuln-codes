PJ_DEF(pj_status_t) pjsip_ua_unregister_dlg( pjsip_user_agent *ua,
					     pjsip_dialog *dlg )
{
    struct dlg_set *dlg_set;
    pjsip_dialog *d;

    /* Sanity-check arguments. */
    PJ_ASSERT_RETURN(ua && dlg, PJ_EINVAL);

    /* Check that dialog has been registered. */
    PJ_ASSERT_RETURN(dlg->dlg_set, PJ_EINVALIDOP);

    /* Lock user agent. */
    pj_mutex_lock(mod_ua.mutex);

    /* Find this dialog from the dialog set. */
    dlg_set = (struct dlg_set*) dlg->dlg_set;
    d = dlg_set->dlg_list.next;
    while (d != (pjsip_dialog*)&dlg_set->dlg_list && d != dlg) {
	d = d->next;
    }

    if (d != dlg) {
	pj_assert(!"Dialog is not registered!");
	pj_mutex_unlock(mod_ua.mutex);
	return PJ_EINVALIDOP;
    }

    /* Remove this dialog from the list. */
    pj_list_erase(dlg);

    /* If dialog list is empty, remove the dialog set from the hash table. */
    if (pj_list_empty(&dlg_set->dlg_list)) {
	pj_hash_set_lower(NULL, mod_ua.dlg_table, dlg->local.info->tag.ptr,
		          (unsigned)dlg->local.info->tag.slen, 
			  dlg->local.tag_hval, NULL);

	/* Return dlg_set to free nodes. */
	pj_list_push_back(&mod_ua.free_dlgset_nodes, dlg_set);
    }

    /* Unlock user agent. */
    pj_mutex_unlock(mod_ua.mutex);

    /* Done. */
    return PJ_SUCCESS;
}