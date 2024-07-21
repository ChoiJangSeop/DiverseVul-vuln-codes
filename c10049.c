static void get_request(GDHCPClient *dhcp_client, struct dhcp_packet *packet)
{
	GDHCPOptionType type;
	GList *list, *value_list;
	char *option_value;
	uint8_t *option;
	uint8_t code;

	for (list = dhcp_client->request_list; list; list = list->next) {
		code = (uint8_t) GPOINTER_TO_INT(list->data);

		option = dhcp_get_option(packet, code);
		if (!option) {
			g_hash_table_remove(dhcp_client->code_value_hash,
						GINT_TO_POINTER((int) code));
			continue;
		}

		type =  dhcp_get_code_type(code);

		option_value = malloc_option_value_string(option, type);
		if (!option_value)
			g_hash_table_remove(dhcp_client->code_value_hash,
						GINT_TO_POINTER((int) code));

		value_list = get_option_value_list(option_value, type);

		g_free(option_value);

		if (!value_list)
			g_hash_table_remove(dhcp_client->code_value_hash,
						GINT_TO_POINTER((int) code));
		else
			g_hash_table_insert(dhcp_client->code_value_hash,
				GINT_TO_POINTER((int) code), value_list);
	}
}