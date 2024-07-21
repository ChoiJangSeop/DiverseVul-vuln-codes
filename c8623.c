ipmi_sdr_print_sensor_mc_locator(struct sdr_record_mc_locator *mc)
{
	char desc[17];

	if (!mc)
		return -1;

	memset(desc, 0, sizeof (desc));
	snprintf(desc, (mc->id_code & 0x1f) + 1, "%s", mc->id_string);

	if (verbose == 0) {
		if (csv_output)
			printf("%s,00h,ok,%d.%d\n",
			       mc->id_code ? desc : "",
			       mc->entity.id, mc->entity.instance);
		else if (sdr_extended) {
			printf("%-16s | 00h | ok  | %2d.%1d | ",
			       mc->id_code ? desc : "",
			       mc->entity.id, mc->entity.instance);

			printf("%s MC @ %02Xh\n",
			       (mc->
				pwr_state_notif & 0x1) ? "Static" : "Dynamic",
			       mc->dev_slave_addr);
		} else {
			printf("%-16s | %s MC @ %02Xh %s | ok\n",
			       mc->id_code ? desc : "",
			       (mc->
				pwr_state_notif & 0x1) ? "Static" : "Dynamic",
			       mc->dev_slave_addr,
			       (mc->pwr_state_notif & 0x1) ? " " : "");
		}

		return 0;	/* done */
	}

	printf("Device ID              : %s\n", mc->id_string);
	printf("Entity ID              : %d.%d (%s)\n",
	       mc->entity.id, mc->entity.instance,
	       val2str(mc->entity.id, entity_id_vals));

	printf("Device Slave Address   : %02Xh\n", mc->dev_slave_addr);
	printf("Channel Number         : %01Xh\n", mc->channel_num);

	printf("ACPI System P/S Notif  : %sRequired\n",
	       (mc->pwr_state_notif & 0x4) ? "" : "Not ");
	printf("ACPI Device P/S Notif  : %sRequired\n",
	       (mc->pwr_state_notif & 0x2) ? "" : "Not ");
	printf("Controller Presence    : %s\n",
	       (mc->pwr_state_notif & 0x1) ? "Static" : "Dynamic");
	printf("Logs Init Agent Errors : %s\n",
	       (mc->global_init & 0x8) ? "Yes" : "No");

	printf("Event Message Gen      : ");
	if (!(mc->global_init & 0x3))
		printf("Enable\n");
	else if ((mc->global_init & 0x3) == 0x1)
		printf("Disable\n");
	else if ((mc->global_init & 0x3) == 0x2)
		printf("Do Not Init Controller\n");
	else
		printf("Reserved\n");

	printf("Device Capabilities\n");
	printf(" Chassis Device        : %s\n",
	       (mc->dev_support & 0x80) ? "Yes" : "No");
	printf(" Bridge                : %s\n",
	       (mc->dev_support & 0x40) ? "Yes" : "No");
	printf(" IPMB Event Generator  : %s\n",
	       (mc->dev_support & 0x20) ? "Yes" : "No");
	printf(" IPMB Event Receiver   : %s\n",
	       (mc->dev_support & 0x10) ? "Yes" : "No");
	printf(" FRU Inventory Device  : %s\n",
	       (mc->dev_support & 0x08) ? "Yes" : "No");
	printf(" SEL Device            : %s\n",
	       (mc->dev_support & 0x04) ? "Yes" : "No");
	printf(" SDR Repository        : %s\n",
	       (mc->dev_support & 0x02) ? "Yes" : "No");
	printf(" Sensor Device         : %s\n",
	       (mc->dev_support & 0x01) ? "Yes" : "No");

	printf("\n");

	return 0;
}