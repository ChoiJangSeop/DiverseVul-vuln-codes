p11_kit_iter_next (P11KitIter *iter)
{
	CK_ULONG batch;
	CK_ULONG count;
	CK_BBOOL matches;
	CK_RV rv;

	return_val_if_fail (iter->iterating, CKR_OPERATION_NOT_INITIALIZED);

	COROUTINE_BEGIN (iter_next);

	iter->object = 0;

	if (iter->match_nothing)
		return finish_iterating (iter, CKR_CANCEL);

	if (!(iter->with_modules || iter->with_slots || iter->with_tokens || iter->with_objects))
		return finish_iterating (iter, CKR_CANCEL);

	/*
	 * If we have outstanding objects, then iterate one through those
	 * Note that we pass each object through the filters, and only
	 * assume it's iterated if it matches
	 */
	while (iter->with_objects && iter->saw_objects < iter->num_objects) {
		iter->object = iter->objects[iter->saw_objects++];

		rv = call_all_filters (iter, &matches);
		if (rv != CKR_OK)
			return finish_iterating (iter, rv);

		if (matches && iter->with_objects) {
			iter->kind = P11_KIT_ITER_KIND_OBJECT;
			COROUTINE_RETURN (iter_next, 1, CKR_OK);
		}
	}

	/* Move to next session, if we have finished searching
	 * objects, or we are looking for modules/slots/tokens */
	if ((iter->with_objects && iter->searched) ||
	    (!iter->with_objects &&
	     (iter->with_modules || iter->with_slots || iter->with_tokens))) {
		/* Use iter->kind as the sentinel to detect the case where
		 * any match (except object) is successful in
		 * move_next_session() */
		do {
			iter->kind = P11_KIT_ITER_KIND_UNKNOWN;
			rv = move_next_session (iter);
			if (rv != CKR_OK)
				return finish_iterating (iter, rv);
			if (iter->kind != P11_KIT_ITER_KIND_UNKNOWN)
				COROUTINE_RETURN (iter_next, 2, CKR_OK);
		} while (iter->move_next_session_state > 0);
	}

	/* Ready to start searching */
	if (iter->with_objects && !iter->searching && !iter->searched) {
		count = p11_attrs_count (iter->match_attrs);
		rv = (iter->module->C_FindObjectsInit) (iter->session, iter->match_attrs, count);
		if (rv != CKR_OK)
			return finish_iterating (iter, rv);
		iter->searching = 1;
		iter->searched = 0;
	}

	/* If we have searched on this session then try to continue */
	if (iter->with_objects && iter->searching) {
		assert (iter->module != NULL);
		assert (iter->session != 0);
		iter->num_objects = 0;
		iter->saw_objects = 0;

		for (;;) {
			if (iter->max_objects - iter->num_objects == 0) {
				CK_OBJECT_HANDLE *objects;

				iter->max_objects = iter->max_objects ? iter->max_objects * 2 : 64;
				objects = realloc (iter->objects, iter->max_objects * sizeof (CK_ULONG));
				return_val_if_fail (objects != NULL, CKR_HOST_MEMORY);
				iter->objects = objects;
			}

			batch = iter->max_objects - iter->num_objects;
			rv = (iter->module->C_FindObjects) (iter->session,
			                                    iter->objects + iter->num_objects,
			                                    batch, &count);
			if (rv != CKR_OK)
				return finish_iterating (iter, rv);

			iter->num_objects += count;

			/*
			 * Done searching on this session, although there are still
			 * objects outstanding, which will be returned on next
			 * iterations.
			 */
			if (batch != count) {
				iter->searching = 0;
				iter->searched = 1;
				(iter->module->C_FindObjectsFinal) (iter->session);
				break;
			}

			if (!iter->preload_results)
				break;
		}
	}

	COROUTINE_END (iter_next);

	/* Try again */
	iter->iter_next_state = 0;
	iter->move_next_session_state = 0;
	iter->kind = P11_KIT_ITER_KIND_UNKNOWN;
	return p11_kit_iter_next (iter);
}