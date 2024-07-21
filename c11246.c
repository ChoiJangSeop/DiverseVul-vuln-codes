void isis_notif_id_len_mismatch(const struct isis_circuit *circuit,
				uint8_t rcv_id_len, const char *raw_pdu,
				size_t raw_pdu_len)
{
	const char *xpath = "/frr-isisd:id-len-mismatch";
	struct list *arguments = yang_data_list_new();
	char xpath_arg[XPATH_MAXLEN];
	struct yang_data *data;
	struct isis_area *area = circuit->area;

	notif_prep_instance_hdr(xpath, area, "default", arguments);
	notif_prepr_iface_hdr(xpath, circuit, arguments);
	snprintf(xpath_arg, sizeof(xpath_arg), "%s/pdu-field-len", xpath);
	data = yang_data_new_uint8(xpath_arg, rcv_id_len);
	listnode_add(arguments, data);
	snprintf(xpath_arg, sizeof(xpath_arg), "%s/raw-pdu", xpath);
	data = yang_data_new(xpath_arg, raw_pdu);
	listnode_add(arguments, data);

	hook_call(isis_hook_id_len_mismatch, circuit, rcv_id_len, raw_pdu,
		  raw_pdu_len);

	nb_notification_send(xpath, arguments);
}