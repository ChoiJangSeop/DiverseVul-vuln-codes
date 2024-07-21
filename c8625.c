ipmi_sdr_print_sensor_fru_locator(struct sdr_record_fru_locator *fru)
{
	char desc[17];

	memset(desc, 0, sizeof (desc));
	snprintf(desc, (fru->id_code & 0x1f) + 1, "%s", fru->id_string);

	if (!verbose) {
		if (csv_output)
			printf("%s,00h,ns,%d.%d\n",
			       fru->id_code ? desc : "",
			       fru->entity.id, fru->entity.instance);
		else if (sdr_extended)
			printf("%-16s | 00h | ns  | %2d.%1d | %s FRU @%02Xh\n",
			       fru->id_code ? desc : "",
			       fru->entity.id, fru->entity.instance,
			       (fru->logical) ? "Logical" : "Physical",
			       fru->device_id);
		else
			printf("%-16s | %s FRU @%02Xh %02x.%x | ok\n",
			       fru->id_code ? desc : "",
			       (fru->logical) ? "Log" : "Phy",
			       fru->device_id,
			       fru->entity.id, fru->entity.instance);

		return 0;
	}

	printf("Device ID              : %s\n", fru->id_string);
	printf("Entity ID              : %d.%d (%s)\n",
	       fru->entity.id, fru->entity.instance,
	       val2str(fru->entity.id, entity_id_vals));

	printf("Device Access Address  : %02Xh\n", fru->dev_slave_addr);
	printf("%s: %02Xh\n",
	       fru->logical ? "Logical FRU Device     " :
	       "Slave Address          ", fru->device_id);
	printf("Channel Number         : %01Xh\n", fru->channel_num);
	printf("LUN.Bus                : %01Xh.%01Xh\n", fru->lun, fru->bus);
	printf("Device Type.Modifier   : %01Xh.%01Xh (%s)\n",
	       fru->dev_type, fru->dev_type_modifier,
	       val2str(fru->dev_type << 8 | fru->dev_type_modifier,
		       entity_device_type_vals));
	printf("OEM                    : %02Xh\n", fru->oem);
	printf("\n");

	return 0;
}