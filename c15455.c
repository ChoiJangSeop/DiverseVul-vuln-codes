static NTSTATUS update_uint32_t_value(TALLOC_CTX *mem_ctx,
				      struct ldb_context *sam_ldb,
				      struct ldb_message *orig,
				      struct ldb_message *dest,
				      const char *attribute,
				      uint32_t value,
				      uint32_t *orig_value)
{
	const struct ldb_val *orig_val;
	uint32_t orig_uint = 0;
	unsigned int flags = 0;
	int ret;
	int error = 0;

	orig_val = ldb_msg_find_ldb_val(orig, attribute);
	if (!orig_val || !orig_val->data) {
		/* add new attribute */
		flags = LDB_FLAG_MOD_ADD;

	} else {
		orig_uint = smb_strtoul((const char *)orig_val->data,
					NULL,
					0,
					&error,
					SMB_STR_STANDARD);
		if (error != 0 || orig_uint != value) {
			/* replace also if can't get value */
			flags = LDB_FLAG_MOD_REPLACE;
		}
	}

	if (flags == 0) {
		/* stored value is identical, nothing to change */
		goto done;
	}

	ret = ldb_msg_add_empty(dest, attribute, flags, NULL);
	if (ret != LDB_SUCCESS) {
		return NT_STATUS_NO_MEMORY;
	}

	ret = samdb_msg_add_uint(sam_ldb, dest, dest, attribute, value);
	if (ret != LDB_SUCCESS) {
		return NT_STATUS_NO_MEMORY;
	}

done:
	if (orig_value) {
		*orig_value = orig_uint;
	}
	return NT_STATUS_OK;
}