static int ldapsrv_load_limits(struct ldapsrv_connection *conn)
{
	TALLOC_CTX *tmp_ctx;
	const char *attrs[] = { "configurationNamingContext", NULL };
	const char *attrs2[] = { "lDAPAdminLimits", NULL };
	struct ldb_message_element *el;
	struct ldb_result *res = NULL;
	struct ldb_dn *basedn;
	struct ldb_dn *conf_dn;
	struct ldb_dn *policy_dn;
	unsigned int i;
	int ret;

	/* set defaults limits in case of failure */
	conn->limits.initial_timeout = 120;
	conn->limits.conn_idle_time = 900;
	conn->limits.max_page_size = 1000;
	conn->limits.max_notifications = 5;
	conn->limits.search_timeout = 120;
	conn->limits.expire_time = (struct timeval) {
		.tv_sec = get_time_t_max(),
	};


	tmp_ctx = talloc_new(conn);
	if (tmp_ctx == NULL) {
		return -1;
	}

	basedn = ldb_dn_new(tmp_ctx, conn->ldb, NULL);
	if (basedn == NULL) {
		goto failed;
	}

	ret = ldb_search(conn->ldb, tmp_ctx, &res, basedn, LDB_SCOPE_BASE, attrs, NULL);
	if (ret != LDB_SUCCESS) {
		goto failed;
	}

	if (res->count != 1) {
		goto failed;
	}

	conf_dn = ldb_msg_find_attr_as_dn(conn->ldb, tmp_ctx, res->msgs[0], "configurationNamingContext");
	if (conf_dn == NULL) {
		goto failed;
	}

	policy_dn = ldb_dn_copy(tmp_ctx, conf_dn);
	ldb_dn_add_child_fmt(policy_dn, "CN=Default Query Policy,CN=Query-Policies,CN=Directory Service,CN=Windows NT,CN=Services");
	if (policy_dn == NULL) {
		goto failed;
	}

	ret = ldb_search(conn->ldb, tmp_ctx, &res, policy_dn, LDB_SCOPE_BASE, attrs2, NULL);
	if (ret != LDB_SUCCESS) {
		goto failed;
	}

	if (res->count != 1) {
		goto failed;
	}

	el = ldb_msg_find_element(res->msgs[0], "lDAPAdminLimits");
	if (el == NULL) {
		goto failed;
	}

	for (i = 0; i < el->num_values; i++) {
		char policy_name[256];
		int policy_value, s;

		s = sscanf((const char *)el->values[i].data, "%255[^=]=%d", policy_name, &policy_value);
		if (s != 2 || policy_value == 0)
			continue;
		if (strcasecmp("InitRecvTimeout", policy_name) == 0) {
			conn->limits.initial_timeout = policy_value;
			continue;
		}
		if (strcasecmp("MaxConnIdleTime", policy_name) == 0) {
			conn->limits.conn_idle_time = policy_value;
			continue;
		}
		if (strcasecmp("MaxPageSize", policy_name) == 0) {
			conn->limits.max_page_size = policy_value;
			continue;
		}
		if (strcasecmp("MaxNotificationPerConn", policy_name) == 0) {
			conn->limits.max_notifications = policy_value;
			continue;
		}
		if (strcasecmp("MaxQueryDuration", policy_name) == 0) {
			conn->limits.search_timeout = policy_value;
			continue;
		}
	}

	return 0;

failed:
	DBG_ERR("Failed to load ldap server query policies\n");
	talloc_free(tmp_ctx);
	return -1;
}