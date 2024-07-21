proxy_list_slots (Proxy *py, Mapping *mappings, unsigned int n_mappings)
{
	CK_FUNCTION_LIST_PTR *f;
	CK_FUNCTION_LIST_PTR funcs;
	CK_SLOT_ID_PTR slots;
	CK_ULONG i, count;
	unsigned int j;
	CK_RV rv = CKR_OK;

	for (f = py->inited; *f; ++f) {
		funcs = *f;
		assert (funcs != NULL);
		slots = NULL;

		/* Ask module for its slots */
		rv = (funcs->C_GetSlotList) (FALSE, NULL, &count);
		if (rv == CKR_OK && count) {
			slots = calloc (count, sizeof (CK_SLOT_ID));
			rv = (funcs->C_GetSlotList) (FALSE, slots, &count);
		}

		if (rv != CKR_OK) {
			free (slots);
			break;
		}

		return_val_if_fail (count == 0 || slots != NULL, CKR_GENERAL_ERROR);

		if (count > 0) {
			Mapping *new_mappings;
			CK_SLOT_ID_PTR new_slots;
			int new_slots_count = 0;

			new_slots = calloc (count, sizeof(CK_SLOT_ID));
			return_val_if_fail (new_slots != NULL, CKR_HOST_MEMORY);
			new_mappings = realloc (py->mappings, sizeof (Mapping) * (py->n_mappings + count));
			return_val_if_fail (new_mappings != NULL, CKR_HOST_MEMORY);
			py->mappings = new_mappings;

			/* Reuse the existing mapping if any */
			for (i = 0; i < count; ++i) {
				for (j = 0; j < n_mappings; ++j) {
					/* cppcheck-suppress nullPointer symbolName=mappings */
					/* false-positive: https://trac.cppcheck.net/ticket/9573 */
					if (mappings[j].funcs == funcs &&
					    mappings[j].real_slot == slots[i]) {
						py->mappings[py->n_mappings].funcs = funcs;
						py->mappings[py->n_mappings].real_slot = slots[i];
						py->mappings[py->n_mappings].wrap_slot =
							mappings[j].wrap_slot;
						++py->n_mappings;
						break;
					}
				}
				if (n_mappings == 0 || j == n_mappings) {
					new_slots[new_slots_count] = slots[i];
					++new_slots_count;
				}
			}

			/* And now add a mapping for each new slot */
			for (i = 0; i < new_slots_count; ++i) {
				++py->last_id;
				py->mappings[py->n_mappings].funcs = funcs;
				py->mappings[py->n_mappings].wrap_slot =
					py->last_id + MAPPING_OFFSET;
				py->mappings[py->n_mappings].real_slot = new_slots[i];
				++py->n_mappings;
			}

			free(new_slots);
		}

		free (slots);
	}
	return rv;
}