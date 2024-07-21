auth_passdb_preinit(struct auth *auth, const struct auth_passdb_settings *set,
		    struct auth_passdb **passdbs)
{
	struct auth_passdb *auth_passdb, **dest;

	auth_passdb = p_new(auth->pool, struct auth_passdb, 1);
	auth_passdb->set = set;
	auth_passdb->skip = auth_passdb_skip_parse(set->skip);
	auth_passdb->result_success =
		auth_db_rule_parse(set->result_success);
	auth_passdb->result_failure =
		auth_db_rule_parse(set->result_failure);
	auth_passdb->result_internalfail =
		auth_db_rule_parse(set->result_internalfail);

	auth_passdb->default_fields_tmpl =
		passdb_template_build(auth->pool, set->default_fields);
	auth_passdb->override_fields_tmpl =
		passdb_template_build(auth->pool, set->override_fields);

	/* for backwards compatibility: */
	if (set->pass)
		auth_passdb->result_success = AUTH_DB_RULE_CONTINUE;

	for (dest = passdbs; *dest != NULL; dest = &(*dest)->next) ;
	*dest = auth_passdb;

	auth_passdb->passdb = passdb_preinit(auth->pool, set);
	/* make sure any %variables in default_fields exist in cache_key */
	if (auth_passdb->passdb->default_cache_key != NULL) {
		auth_passdb->cache_key =
			p_strconcat(auth->pool, auth_passdb->passdb->default_cache_key,
				set->default_fields, NULL);
	}
	else {
		auth_passdb->cache_key = NULL;
	}
}