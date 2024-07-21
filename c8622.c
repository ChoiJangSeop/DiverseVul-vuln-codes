ipmi_sdr_print_sensor_generic_locator(struct sdr_record_generic_locator *dev)
{
	char desc[17];

	memset(desc, 0, sizeof (desc));
	snprintf(desc, (dev->id_code & 0x1f) + 1, "%s", dev->id_string);

	if (!verbose) {
		if (csv_output)
			printf("%s,00h,ns,%d.%d\n",
			       dev->id_code ? desc : "",
			       dev->entity.id, dev->entity.instance);
		else if (sdr_extended)
			printf
			    ("%-16s | 00h | ns  | %2d.%1d | Generic Device @%02Xh:%02Xh.%1d\n",
			     dev->id_code ? desc : "", dev->entity.id,
			     dev->entity.instance, dev->dev_access_addr,
			     dev->dev_slave_addr, dev->oem);
		else
			printf("%-16s | Generic @%02X:%02X.%-2d | ok\n",
			       dev->id_code ? desc : "",
			       dev->dev_access_addr,
			       dev->dev_slave_addr, dev->oem);

		return 0;
	}

	printf("Device ID              : %s\n", dev->id_string);
	printf("Entity ID              : %d.%d (%s)\n",
	       dev->entity.id, dev->entity.instance,
	       val2str(dev->entity.id, entity_id_vals));

	printf("Device Access Address  : %02Xh\n", dev->dev_access_addr);
	printf("Device Slave Address   : %02Xh\n", dev->dev_slave_addr);
	printf("Address Span           : %02Xh\n", dev->addr_span);
	printf("Channel Number         : %01Xh\n", dev->channel_num);
	printf("LUN.Bus                : %01Xh.%01Xh\n", dev->lun, dev->bus);
	printf("Device Type.Modifier   : %01Xh.%01Xh (%s)\n",
	       dev->dev_type, dev->dev_type_modifier,
	       val2str(dev->dev_type << 8 | dev->dev_type_modifier,
		       entity_device_type_vals));
	printf("OEM                    : %02Xh\n", dev->oem);
	printf("\n");

	return 0;
}