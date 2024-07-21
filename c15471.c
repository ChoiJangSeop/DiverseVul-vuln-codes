int partition_create(struct ldb_module *module, struct ldb_request *req)
{
	unsigned int i;
	int ret;
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	struct ldb_request *mod_req, *last_req = req;
	struct ldb_message *mod_msg;
	struct partition_private_data *data;
	struct dsdb_partition *partition = NULL;
	const char *casefold_dn;
	bool new_partition = false;

	/* Check if this is already a partition */

	struct dsdb_create_partition_exop *ex_op = talloc_get_type(req->op.extended.data, struct dsdb_create_partition_exop);
	struct ldb_dn *dn = ex_op->new_dn;

	data = talloc_get_type(ldb_module_get_private(module), struct partition_private_data);
	if (!data) {
		/* We are not going to create a partition before we are even set up */
		return LDB_ERR_UNWILLING_TO_PERFORM;
	}

	/* see if we are still up-to-date */
	ret = partition_reload_if_required(module, data, req);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	for (i=0; data->partitions && data->partitions[i]; i++) {
		if (ldb_dn_compare(data->partitions[i]->ctrl->dn, dn) == 0) {
			partition = data->partitions[i];
		}
	}

	if (!partition) {
		char *filename;
		char *partition_record;
		new_partition = true;
		mod_msg = ldb_msg_new(req);
		if (!mod_msg) {
			return ldb_oom(ldb);
		}
		
		mod_msg->dn = ldb_dn_new(mod_msg, ldb, DSDB_PARTITION_DN);
		ret = ldb_msg_add_empty(mod_msg, DSDB_PARTITION_ATTR, LDB_FLAG_MOD_ADD, NULL);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		
		casefold_dn = ldb_dn_get_casefold(dn);
		
		{
			char *escaped;
			const char *p, *sam_name;
			sam_name = strrchr((const char *)ldb_get_opaque(ldb, "ldb_url"), '/');
			if (!sam_name) {
				return ldb_operr(ldb);
			}
			sam_name++;

			for (p = casefold_dn; *p; p++) {
				/* We have such a strict check because
				 * I don't want shell metacharacters
				 * in the file name, nor ../, but I do
				 * want it to be easily typed if SAFE
				 * to do so */
				if (!(isalnum(*p) || *p == ' ' || *p == '=' || *p == ',')) {
					break;
				}
			}
			if (*p) {
				escaped = rfc1738_escape_part(mod_msg, casefold_dn);
				if (!escaped) {
					return ldb_oom(ldb);
				}
				filename = talloc_asprintf(mod_msg, "%s.d/%s.ldb", sam_name, escaped);
				talloc_free(escaped);
			} else {
				filename = talloc_asprintf(mod_msg, "%s.d/%s.ldb", sam_name, casefold_dn);
			}

			if (!filename) {
				return ldb_oom(ldb);
			}
		}
		partition_record = talloc_asprintf(mod_msg, "%s:%s", casefold_dn, filename);

		ret = ldb_msg_add_steal_string(mod_msg, DSDB_PARTITION_ATTR, partition_record);
		if (ret != LDB_SUCCESS) {
			return ret;
		}

		if (ldb_request_get_control(req, DSDB_CONTROL_PARTIAL_REPLICA)) {
			/* this new partition is a partial replica */
			ret = ldb_msg_add_empty(mod_msg, "partialReplica", LDB_FLAG_MOD_ADD, NULL);
			if (ret != LDB_SUCCESS) {
				return ret;
			}
			ret = ldb_msg_add_fmt(mod_msg, "partialReplica", "%s", ldb_dn_get_linearized(dn));
			if (ret != LDB_SUCCESS) {
				return ret;
			}
		}
		
		/* Perform modify on @PARTITION record */
		ret = ldb_build_mod_req(&mod_req, ldb, req, mod_msg, NULL, NULL, 
					ldb_op_default_callback, req);
		LDB_REQ_SET_LOCATION(mod_req);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		
		last_req = mod_req;

		ret = ldb_next_request(module, mod_req);
		if (ret == LDB_SUCCESS) {
			ret = ldb_wait(mod_req->handle, LDB_WAIT_ALL);
		}
		
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		
		/* Make a partition structure for this new partition, so we can copy in the template structure */ 
		ret = new_partition_from_dn(ldb, data, req, ldb_dn_copy(req, dn),
					    filename, data->backend_db_store,
					    &partition);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		talloc_steal(partition, partition_record);
		partition->orig_record = data_blob_string_const(partition_record);
	}
	
	ret = new_partition_set_replicated_metadata(ldb, module, last_req, data, partition);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	
	if (new_partition) {
		ret = add_partition_to_data(ldb, data, partition);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
	}

	/* send request done */
	return ldb_module_done(req, NULL, NULL, LDB_SUCCESS);
}