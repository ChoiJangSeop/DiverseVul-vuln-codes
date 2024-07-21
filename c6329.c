func_exists (struct _ESExp *f,
             gint argc,
             struct _ESExpResult **argv,
             gpointer data)
{
	EBookBackendLDAPSExpData *ldap_data = data;
	ESExpResult *r;

	if (argc == 1
	    && argv[0]->type == ESEXP_RES_STRING) {
		gchar *propname = argv[0]->value.string;

		if (!strcmp (propname, "x-evolution-any-field")) {
			gint i;
			gint query_length;
			gchar *big_query;
			gchar *match_str;

			match_str = g_strdup ("=*)");

			query_length = 3; /* strlen ("(|") + strlen (")") */

			for (i = 0; i < G_N_ELEMENTS (prop_info); i++) {
				query_length += 1 /* strlen ("(") */ + strlen (prop_info[i].ldap_attr) + strlen (match_str);
			}

			big_query = g_malloc0 (query_length + 1);
			strcat (big_query, "(|");
			for (i = 0; i < G_N_ELEMENTS (prop_info); i++) {
				if (!(prop_info[i].prop_type & PROP_WRITE_ONLY) &&
				    (ldap_data->bl->priv->evolutionPersonSupported ||
				     !(prop_info[i].prop_type & PROP_EVOLVE)) &&
				    (ldap_data->bl->priv->calEntrySupported ||
				     !(prop_info[i].prop_type & PROP_CALENTRY))) {
					strcat (big_query, "(");
					strcat (big_query, prop_info[i].ldap_attr);
					strcat (big_query, match_str);
				}
			}
			strcat (big_query, ")");

			ldap_data->list = g_list_prepend (ldap_data->list, big_query);

			g_free (match_str);
		}
		else {
			const gchar *ldap_attr = query_prop_to_ldap (propname, ldap_data->bl->priv->evolutionPersonSupported, ldap_data->bl->priv->calEntrySupported);

			if (ldap_attr)
				ldap_data->list = g_list_prepend (
					ldap_data->list,
					g_strdup_printf (
						"(%s=*)", ldap_attr));
		}
	}

	r = e_sexp_result_new (f, ESEXP_RES_BOOL);
	r->value.boolean = FALSE;

	return r;
}