vlv_copy_down_controls(TALLOC_CTX *mem_ctx, struct ldb_control **controls)
{

	struct ldb_control **new_controls;
	unsigned int i, j, num_ctrls;
	if (controls == NULL) {
		return NULL;
	}

	for (num_ctrls = 0; controls[num_ctrls]; num_ctrls++);

	new_controls = talloc_array(mem_ctx, struct ldb_control *, num_ctrls);
	if (new_controls == NULL) {
		return NULL;
	}

	for (j = 0, i = 0; i < (num_ctrls); i++) {
		struct ldb_control *control = controls[i];
		if (control->oid == NULL) {
			break;
		}
		/*
		 * Do not re-use VLV, nor the server-sort, both are
		 * already handled here.
		 */
		if (strcmp(control->oid, LDB_CONTROL_VLV_REQ_OID) == 0 ||
		    strcmp(control->oid, LDB_CONTROL_SERVER_SORT_OID) == 0) {
			continue;
		}
		/*
		 * ASQ changes everything, do not copy it down for the
		 * per-GUID search
		 */
		if (strcmp(control->oid, LDB_CONTROL_ASQ_OID) == 0) {
			continue;
		}
		new_controls[j] = talloc_steal(new_controls, control);
		j++;
	}
	new_controls[j] = NULL;
	return new_controls;
}