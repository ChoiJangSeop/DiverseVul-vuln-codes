void isis_notif_authentication_failure(const struct isis_circuit *circuit,
				       const char *raw_pdu, size_t raw_pdu_len)
{
	const char *xpath = "/frr-isisd:authentication-failure";
	struct list *arguments = yang_data_list_new();
	char xpath_arg[XPATH_MAXLEN];
	struct yang_data *data;
	struct isis_area *area = circuit->area;

	notif_prep_instance_hdr(xpath, area, "default", arguments);
	notif_prepr_iface_hdr(xpath, circuit, arguments);
	snprintf(xpath_arg, sizeof(xpath_arg), "%s/raw-pdu", xpath);
	data = yang_data_new(xpath_arg, raw_pdu);
	listnode_add(arguments, data);

	hook_call(isis_hook_authentication_failure, circuit, raw_pdu,
		  raw_pdu_len);

	nb_notification_send(xpath, arguments);
}