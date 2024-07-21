WERROR dns_common_replace(struct ldb_context *samdb,
			  TALLOC_CTX *mem_ctx,
			  struct ldb_dn *dn,
			  bool needs_add,
			  uint32_t serial,
			  struct dnsp_DnssrvRpcRecord *records,
			  uint16_t rec_count)
{
	const struct timeval start = timeval_current();
	struct ldb_message_element *el;
	uint16_t i;
	int ret;
	WERROR werr;
	struct ldb_message *msg = NULL;
	bool was_tombstoned = false;
	bool become_tombstoned = false;
	struct ldb_dn *zone_dn = NULL;
	struct dnsserver_zoneinfo *zoneinfo = NULL;
	uint32_t t;

	msg = ldb_msg_new(mem_ctx);
	W_ERROR_HAVE_NO_MEMORY(msg);

	msg->dn = dn;

	zone_dn = ldb_dn_copy(mem_ctx, dn);
	if (zone_dn == NULL) {
		werr = WERR_NOT_ENOUGH_MEMORY;
		goto exit;
	}
	if (!ldb_dn_remove_child_components(zone_dn, 1)) {
		werr = DNS_ERR(SERVER_FAILURE);
		goto exit;
	}
	zoneinfo = talloc(mem_ctx, struct dnsserver_zoneinfo);
	if (zoneinfo == NULL) {
		werr = WERR_NOT_ENOUGH_MEMORY;
		goto exit;
	}
	werr = dns_get_zone_properties(samdb, mem_ctx, zone_dn, zoneinfo);
	if (W_ERROR_EQUAL(DNS_ERR(NOTZONE), werr)) {
		/*
		 * We only got zoneinfo for aging so if we didn't find any
		 * properties then just disable aging and keep going.
		 */
		zoneinfo->fAging = 0;
	} else if (!W_ERROR_IS_OK(werr)) {
		goto exit;
	}

	werr = check_name_list(mem_ctx, rec_count, records);
	if (!W_ERROR_IS_OK(werr)) {
		goto exit;
	}

	ret = ldb_msg_add_empty(msg, "dnsRecord", LDB_FLAG_MOD_REPLACE, &el);
	if (ret != LDB_SUCCESS) {
		werr = DNS_ERR(SERVER_FAILURE);
		goto exit;
	}

	/*
	 * we have at least one value,
	 * which might be used for the tombstone marker
	 */
	el->values = talloc_zero_array(el, struct ldb_val, MAX(1, rec_count));
	if (el->values == NULL) {
		werr = WERR_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	if (rec_count > 1) {
		/*
		 * We store a sorted list with the high wType values first
		 * that's what windows does. It also simplifies the
		 * filtering of DNS_TYPE_TOMBSTONE records
		 */
		TYPESAFE_QSORT(records, rec_count, rec_cmp);
	}

	for (i = 0; i < rec_count; i++) {
		struct ldb_val *v = &el->values[el->num_values];
		enum ndr_err_code ndr_err;

		if (records[i].wType == DNS_TYPE_TOMBSTONE) {
			/*
			 * There are two things that could be going on here.
			 *
			 * 1. We use a tombstone with EntombedTime == 0 for
			 * passing deletion messages through the stack, and
			 * this is the place we filter them out to perform
			 * that deletion.
			 *
			 * 2. This node is tombstoned, with no records except
			 * for a single tombstone, and it is just waiting to
			 * disappear. In this case, unless the caller has
			 * added a record, rec_count should be 1, and
			 * el->num_values will end up at 0, and we will make
			 * no changes. But if the caller has added a record,
			 * we need to un-tombstone the node.
			 *
			 * It is not possible to add an explicit tombstone
			 * record.
			 */
			if (records[i].data.EntombedTime != 0) {
				was_tombstoned = true;
			}
			continue;
		}

		if (zoneinfo->fAging == 1 && records[i].dwTimeStamp != 0) {
			t = unix_to_dns_timestamp(time(NULL));
			if (t - records[i].dwTimeStamp >
			    zoneinfo->dwNoRefreshInterval) {
				records[i].dwTimeStamp = t;
			}
		}

		records[i].dwSerial = serial;
		ndr_err = ndr_push_struct_blob(v, el->values, &records[i],
				(ndr_push_flags_fn_t)ndr_push_dnsp_DnssrvRpcRecord);
		if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
			DEBUG(0, ("Failed to push dnsp_DnssrvRpcRecord\n"));
			werr = DNS_ERR(SERVER_FAILURE);
			goto exit;
		}
		el->num_values++;
	}

	if (needs_add) {
		/*
		 * This is a new dnsNode, which simplifies everything as we
		 * know there is nothing to delete or change. We add the
		 * records and get out.
		 */
		if (el->num_values == 0) {
			werr = WERR_OK;
			goto exit;
		}

		ret = ldb_msg_add_string(msg, "objectClass", "dnsNode");
		if (ret != LDB_SUCCESS) {
			werr = DNS_ERR(SERVER_FAILURE);
			goto exit;
		}

		ret = ldb_add(samdb, msg);
		if (ret != LDB_SUCCESS) {
			werr = DNS_ERR(SERVER_FAILURE);
			goto exit;
		}

		werr = WERR_OK;
		goto exit;
	}

	if (el->num_values == 0) {
		/*
		 * We get here if there are no records or all the records were
		 * tombstones.
		 */
		struct dnsp_DnssrvRpcRecord tbs;
		struct ldb_val *v = &el->values[el->num_values];
		enum ndr_err_code ndr_err;
		struct timeval tv;

		if (was_tombstoned) {
			/*
			 * This is already a tombstoned object.
			 * Just leave it instead of updating the time stamp.
			 */
			werr = WERR_OK;
			goto exit;
		}

		tv = timeval_current();
		tbs = (struct dnsp_DnssrvRpcRecord) {
			.wType = DNS_TYPE_TOMBSTONE,
			.dwSerial = serial,
			.data.EntombedTime = timeval_to_nttime(&tv),
		};

		ndr_err = ndr_push_struct_blob(v, el->values, &tbs,
				(ndr_push_flags_fn_t)ndr_push_dnsp_DnssrvRpcRecord);
		if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
			DEBUG(0, ("Failed to push dnsp_DnssrvRpcRecord\n"));
			werr = DNS_ERR(SERVER_FAILURE);
			goto exit;
		}
		el->num_values++;

		become_tombstoned = true;
	}

	if (was_tombstoned || become_tombstoned) {
		ret = ldb_msg_add_empty(msg, "dNSTombstoned",
					LDB_FLAG_MOD_REPLACE, NULL);
		if (ret != LDB_SUCCESS) {
			werr = DNS_ERR(SERVER_FAILURE);
			goto exit;
		}

		ret = ldb_msg_add_fmt(msg, "dNSTombstoned", "%s",
				      become_tombstoned ? "TRUE" : "FALSE");
		if (ret != LDB_SUCCESS) {
			werr = DNS_ERR(SERVER_FAILURE);
			goto exit;
		}
	}

	ret = ldb_modify(samdb, msg);
	if (ret != LDB_SUCCESS) {
		NTSTATUS nt = dsdb_ldb_err_to_ntstatus(ret);
		werr = ntstatus_to_werror(nt);
		goto exit;
	}

	werr = WERR_OK;
exit:
	talloc_free(msg);
	DNS_COMMON_LOG_OPERATION(
		win_errstr(werr),
		&start,
		NULL,
		dn == NULL ? NULL : ldb_dn_get_linearized(dn),
		NULL);
	return werr;
}