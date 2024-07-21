void isis_notif_lsp_error(const struct isis_circuit *circuit,
			  const uint8_t *lsp_id, const char *raw_pdu,
			  size_t raw_pdu_len,
			  __attribute__((unused)) uint32_t offset,
			  __attribute__((unused)) uint8_t tlv_type)
{
	const char *xpath = "/frr-isisd:lsp-error-detected";
	struct list *arguments = yang_data_list_new();
	char xpath_arg[XPATH_MAXLEN];
	struct yang_data *data;
	struct isis_area *area = circuit->area;

	notif_prep_instance_hdr(xpath, area, "default", arguments);
	notif_prepr_iface_hdr(xpath, circuit, arguments);
	snprintf(xpath_arg, sizeof(xpath_arg), "%s/lsp-id", xpath);
	data = yang_data_new_string(xpath_arg, rawlspid_print(lsp_id));
	listnode_add(arguments, data);
	snprintf(xpath_arg, sizeof(xpath_arg), "%s/raw-pdu", xpath);
	data = yang_data_new(xpath_arg, raw_pdu);
	listnode_add(arguments, data);
	/* ignore offset and tlv_type which cannot be set properly */

	hook_call(isis_hook_lsp_error, circuit, lsp_id, raw_pdu, raw_pdu_len);

	nb_notification_send(xpath, arguments);
}