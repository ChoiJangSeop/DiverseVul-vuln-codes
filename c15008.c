static int acl_modify(struct ldb_module *module, struct ldb_request *req)
{
	int ret;
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	const struct dsdb_schema *schema;
	unsigned int i;
	const struct dsdb_class *objectclass;
	struct ldb_result *acl_res;
	struct security_descriptor *sd;
	struct dom_sid *sid = NULL;
	struct ldb_control *as_system;
	struct ldb_control *is_undelete;
	bool userPassword;
	bool password_rights_checked = false;
	TALLOC_CTX *tmp_ctx;
	const struct ldb_message *msg = req->op.mod.message;
	static const char *acl_attrs[] = {
		"nTSecurityDescriptor",
		"objectClass",
		"objectSid",
		NULL
	};
	struct acl_callback_context *context = NULL;
	struct ldb_request *new_req = NULL;
	struct  dsdb_control_password_acl_validation *pav = NULL;
	struct ldb_control **controls = NULL;

	if (ldb_dn_is_special(msg->dn)) {
		return ldb_next_request(module, req);
	}

	as_system = ldb_request_get_control(req, LDB_CONTROL_AS_SYSTEM_OID);
	if (as_system != NULL) {
		as_system->critical = 0;
	}

	is_undelete = ldb_request_get_control(req, DSDB_CONTROL_RESTORE_TOMBSTONE_OID);

	/* Don't print this debug statement if elements[0].name is going to be NULL */
	if (msg->num_elements > 0) {
		DEBUG(10, ("ldb:acl_modify: %s\n", msg->elements[0].name));
	}
	if (dsdb_module_am_system(module) || as_system) {
		return ldb_next_request(module, req);
	}

	tmp_ctx = talloc_new(req);
	if (tmp_ctx == NULL) {
		return ldb_oom(ldb);
	}

	ret = dsdb_module_search_dn(module, tmp_ctx, &acl_res, msg->dn,
				    acl_attrs,
				    DSDB_FLAG_NEXT_MODULE |
				    DSDB_FLAG_AS_SYSTEM |
				    DSDB_SEARCH_SHOW_RECYCLED,
				    req);

	if (ret != LDB_SUCCESS) {
		goto fail;
	}

	userPassword = dsdb_user_password_support(module, req, req);

	schema = dsdb_get_schema(ldb, tmp_ctx);
	if (!schema) {
		talloc_free(tmp_ctx);
		return ldb_error(ldb, LDB_ERR_OPERATIONS_ERROR,
				 "acl_modify: Error obtaining schema.");
	}

	ret = dsdb_get_sd_from_ldb_message(ldb, tmp_ctx, acl_res->msgs[0], &sd);
	if (ret != LDB_SUCCESS) {
		talloc_free(tmp_ctx);
		return ldb_error(ldb, LDB_ERR_OPERATIONS_ERROR,
				 "acl_modify: Error retrieving security descriptor.");
	}
	/* Theoretically we pass the check if the object has no sd */
	if (!sd) {
		goto success;
	}

	objectclass = dsdb_get_structural_oc_from_msg(schema, acl_res->msgs[0]);
	if (!objectclass) {
		talloc_free(tmp_ctx);
		return ldb_error(ldb, LDB_ERR_OPERATIONS_ERROR,
				 "acl_modify: Error retrieving object class for GUID.");
	}
	sid = samdb_result_dom_sid(req, acl_res->msgs[0], "objectSid");
	for (i=0; i < msg->num_elements; i++) {
		const struct ldb_message_element *el = &msg->elements[i];
		const struct dsdb_attribute *attr;

		/*
		 * This basic attribute existence check with the right errorcode
		 * is needed since this module is the first one which requests
		 * schema attribute information.
		 * The complete attribute checking is done in the
		 * "objectclass_attrs" module behind this one.
		 *
		 * NOTE: "clearTextPassword" is not defined in the schema.
		 */
		attr = dsdb_attribute_by_lDAPDisplayName(schema, el->name);
		if (!attr && ldb_attr_cmp("clearTextPassword", el->name) != 0) {
			ldb_asprintf_errstring(ldb, "acl_modify: attribute '%s' "
					       "on entry '%s' was not found in the schema!",
					       req->op.mod.message->elements[i].name,
				       ldb_dn_get_linearized(req->op.mod.message->dn));
			ret =  LDB_ERR_NO_SUCH_ATTRIBUTE;
			goto fail;
		}

		if (ldb_attr_cmp("nTSecurityDescriptor", el->name) == 0) {
			uint32_t sd_flags = dsdb_request_sd_flags(req, NULL);
			uint32_t access_mask = 0;

			if (sd_flags & (SECINFO_OWNER|SECINFO_GROUP)) {
				access_mask |= SEC_STD_WRITE_OWNER;
			}
			if (sd_flags & SECINFO_DACL) {
				access_mask |= SEC_STD_WRITE_DAC;
			}
			if (sd_flags & SECINFO_SACL) {
				access_mask |= SEC_FLAG_SYSTEM_SECURITY;
			}

			ret = acl_check_access_on_attribute(module,
							    tmp_ctx,
							    sd,
							    sid,
							    access_mask,
							    attr,
							    objectclass);
			if (ret != LDB_SUCCESS) {
				ldb_asprintf_errstring(ldb_module_get_ctx(module),
						       "Object %s has no write dacl access\n",
						       ldb_dn_get_linearized(msg->dn));
				dsdb_acl_debug(sd,
					       acl_user_token(module),
					       msg->dn,
					       true,
					       10);
				ret = LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS;
				goto fail;
			}
		} else if (ldb_attr_cmp("member", el->name) == 0) {
			ret = acl_check_self_membership(tmp_ctx,
							module,
							req,
							sd,
							sid,
							attr,
							objectclass);
			if (ret != LDB_SUCCESS) {
				goto fail;
			}
		} else if (ldb_attr_cmp("dBCSPwd", el->name) == 0) {
			/* this one is not affected by any rights, we should let it through
			   so that passwords_hash returns the correct error */
			continue;
		} else if (ldb_attr_cmp("unicodePwd", el->name) == 0 ||
			   (userPassword && ldb_attr_cmp("userPassword", el->name) == 0) ||
			   ldb_attr_cmp("clearTextPassword", el->name) == 0) {
			/*
			 * Ideally we would do the acl_check_password_rights
			 * before we checked the other attributes, i.e. in a
			 * loop before the current one.
			 * Have not done this as yet in order to limit the size
			 * of the change. To limit the possibility of breaking
			 * the ACL logic.
			 */
			if (password_rights_checked) {
				continue;
			}
			ret = acl_check_password_rights(tmp_ctx,
							module,
							req,
							sd,
							sid,
							objectclass,
							userPassword,
							&pav);
			if (ret != LDB_SUCCESS) {
				goto fail;
			}
			password_rights_checked = true;
		} else if (ldb_attr_cmp("servicePrincipalName", el->name) == 0) {
			ret = acl_check_spn(tmp_ctx,
					    module,
					    req,
					    el,
					    sd,
					    sid,
					    attr,
					    objectclass);
			if (ret != LDB_SUCCESS) {
				goto fail;
			}
		} else if (is_undelete != NULL && (ldb_attr_cmp("isDeleted", el->name) == 0)) {
			/*
			 * in case of undelete op permissions on
			 * isDeleted are irrelevant and
			 * distinguishedName is removed by the
			 * tombstone_reanimate module
			 */
			continue;
		} else {
			ret = acl_check_access_on_attribute(module,
							    tmp_ctx,
							    sd,
							    sid,
							    SEC_ADS_WRITE_PROP,
							    attr,
							    objectclass);
			if (ret != LDB_SUCCESS) {
				ldb_asprintf_errstring(ldb_module_get_ctx(module),
						       "Object %s has no write property access\n",
						       ldb_dn_get_linearized(msg->dn));
				dsdb_acl_debug(sd,
					       acl_user_token(module),
					       msg->dn,
					       true,
					       10);
				ret = LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS;
				goto fail;
			}
		}
	}

success:
	talloc_free(tmp_ctx);
	context = talloc_zero(req, struct acl_callback_context);

	if (context == NULL) {
		return ldb_oom(ldb);
	}
	context->request = req;
	context->module  = module;
	ret = ldb_build_mod_req(
		&new_req,
		ldb,
		req,
		req->op.mod.message,
		req->controls,
		context,
		acl_callback,
		req);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	return ldb_next_request(module, new_req);
fail:
	talloc_free(tmp_ctx);
	/*
	 * We copy the pav into the result, so that the password reset
	 * logging code in audit_log can log failed password reset attempts.
	 */
	if (pav) {
		struct ldb_control *control = NULL;

		controls = talloc_zero_array(req, struct ldb_control *, 2);
		if (controls == NULL) {
			return ldb_oom(ldb);
		}

		control = talloc(controls, struct ldb_control);

		if (control == NULL) {
			return ldb_oom(ldb);
		}

		control->oid= talloc_strdup(
			control,
			DSDB_CONTROL_PASSWORD_ACL_VALIDATION_OID);
		if (control->oid == NULL) {
			return ldb_oom(ldb);
		}
		control->critical	= false;
		control->data	= pav;
		*controls = control;
	}
	return ldb_module_done(req, controls, NULL, ret);
}