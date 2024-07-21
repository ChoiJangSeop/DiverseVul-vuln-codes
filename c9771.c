move_next_session (P11KitIter *iter)
{
	CK_ULONG session_flags;
	CK_ULONG num_slots;
	CK_INFO minfo;
	CK_RV rv;

	COROUTINE_BEGIN (move_next_session);

	finish_slot (iter);

	/* If we have no more slots, then move to next module */
	while (iter->saw_slots >= iter->num_slots) {
		finish_module (iter);

		/* Iter is finished */
		if (iter->modules->num == 0)
			return finish_iterating (iter, CKR_CANCEL);

		iter->module = iter->modules->elem[0];
		p11_array_remove (iter->modules, 0);

		/* Skip module if it doesn't match uri */
		assert (iter->module != NULL);
		rv = (iter->module->C_GetInfo) (&minfo);
		if (rv != CKR_OK || !p11_match_uri_module_info (&iter->match_module, &minfo))
			continue;

		if (iter->with_modules) {
			iter->kind = P11_KIT_ITER_KIND_MODULE;
			COROUTINE_RETURN (move_next_session, 1, CKR_OK);
		}

		if (iter->with_slots || iter->with_tokens || iter->with_objects) {
			CK_SLOT_ID *slots;

			rv = (iter->module->C_GetSlotList) (CK_TRUE, NULL, &num_slots);
			if (rv != CKR_OK)
				return finish_iterating (iter, rv);

			slots = realloc (iter->slots, sizeof (CK_SLOT_ID) * (num_slots + 1));
			return_val_if_fail (slots != NULL, CKR_HOST_MEMORY);
			iter->slots = slots;

			rv = (iter->module->C_GetSlotList) (CK_TRUE, iter->slots, &num_slots);
			if (rv != CKR_OK)
				return finish_iterating (iter, rv);

			iter->num_slots = num_slots;
			assert (iter->saw_slots == 0);
		}
	}

	/* Move to the next slot, and open a session on it */
	while ((iter->with_slots || iter->with_tokens || iter->with_objects) &&
	       iter->saw_slots < iter->num_slots) {
		iter->slot = iter->slots[iter->saw_slots++];

		assert (iter->module != NULL);
		if (iter->match_slot_id != (CK_SLOT_ID)-1 && iter->slot != iter->match_slot_id)
			continue;
		rv = (iter->module->C_GetSlotInfo) (iter->slot, &iter->slot_info);
		if (rv != CKR_OK || !p11_match_uri_slot_info (&iter->match_slot, &iter->slot_info))
			continue;
		if (iter->with_slots) {
			iter->kind = P11_KIT_ITER_KIND_SLOT;
			COROUTINE_RETURN (move_next_session, 2, CKR_OK);
		}
		rv = (iter->module->C_GetTokenInfo) (iter->slot, &iter->token_info);
		if (rv != CKR_OK || !p11_match_uri_token_info (&iter->match_token, &iter->token_info))
			continue;
		if (iter->with_tokens) {
			iter->kind = P11_KIT_ITER_KIND_TOKEN;
			COROUTINE_RETURN (move_next_session, 3, CKR_OK);
		}

		session_flags = CKF_SERIAL_SESSION;

		/* Skip if the read/write on a read-only token */
		if (iter->want_writable && (iter->token_info.flags & CKF_WRITE_PROTECTED) == 0)
			session_flags |= CKF_RW_SESSION;

		rv = (iter->module->C_OpenSession) (iter->slot, session_flags,
		                                    NULL, NULL, &iter->session);
		if (rv != CKR_OK)
			return finish_iterating (iter, rv);

		if (iter->session != 0) {
			iter->move_next_session_state = 0;
			iter->kind = P11_KIT_ITER_KIND_UNKNOWN;
			return CKR_OK;
		}
	}

	COROUTINE_END (move_next_session);

	/* Otherwise try again */
	iter->move_next_session_state = 0;
	return move_next_session (iter);
}