e_ews_autodiscover_ws_url_finish (CamelEwsSettings *settings,
                                  GAsyncResult *result,
                                  GError **error)
{
	GSimpleAsyncResult *simple;
	struct _autodiscover_data *ad;

	g_return_val_if_fail (
		g_simple_async_result_is_valid (
		result, G_OBJECT (settings),
		e_ews_autodiscover_ws_url), FALSE);

	simple = G_SIMPLE_ASYNC_RESULT (result);
	ad = g_simple_async_result_get_op_res_gpointer (simple);

	if (g_simple_async_result_propagate_error (simple, error))
		return FALSE;

	g_warn_if_fail (ad->as_url != NULL);
	g_warn_if_fail (ad->oab_url != NULL);

	camel_ews_settings_set_hosturl (settings, ad->as_url);

	if (!has_suffix_icmp (ad->oab_url, "oab.xml")) {
		gchar *tmp;

		if (g_str_has_suffix (ad->oab_url, "/"))
			tmp = g_strconcat (ad->oab_url, "oab.xml", NULL);
		else
			tmp = g_strconcat (ad->oab_url, "/", "oab.xml", NULL);

		camel_ews_settings_set_oaburl (settings, tmp);
		g_free (tmp);
	} else {
		camel_ews_settings_set_oaburl (settings, ad->oab_url);
	}

	return TRUE;
}