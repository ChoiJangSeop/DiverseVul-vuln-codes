ipmi_fru_print(struct ipmi_intf * intf, struct sdr_record_fru_locator * fru)
{
	char desc[17];
	uint8_t  bridged_request = 0;
	uint32_t save_addr;
	uint32_t save_channel;
	int rc = 0;

	if (!fru)
		return __ipmi_fru_print(intf, 0);

	/* Logical FRU Device
	*  dev_type == 0x10
	*  modifier
	*   0x00 = IPMI FRU Inventory
	*   0x01 = DIMM Memory ID
	*   0x02 = IPMI FRU Inventory
	*   0x03 = System Processor FRU
	*   0xff = unspecified
	*
	* EEPROM 24C01 or equivalent
	*  dev_type >= 0x08 && dev_type <= 0x0f
	*  modifier
	*   0x00 = unspecified
	*   0x01 = DIMM Memory ID
	*   0x02 = IPMI FRU Inventory
	*   0x03 = System Processor Cartridge
	*/
	if (fru->dev_type != 0x10 &&
		(fru->dev_type_modifier != 0x02 ||
		fru->dev_type < 0x08 || fru->dev_type > 0x0f))
		return -1;

	if (fru->dev_slave_addr == IPMI_BMC_SLAVE_ADDR &&
		fru->device_id == 0)
		return 0;

	memset(desc, 0, sizeof(desc));
	memcpy(desc, fru->id_string, fru->id_code & 0x01f);
	desc[fru->id_code & 0x01f] = 0;
	printf("FRU Device Description : %s (ID %d)\n", desc, fru->device_id);

	switch (fru->dev_type_modifier) {
	case 0x00:
	case 0x02:
		if (BRIDGE_TO_SENSOR(intf, fru->dev_slave_addr,
					   fru->channel_num)) {
			bridged_request = 1;
			save_addr = intf->target_addr;
			intf->target_addr = fru->dev_slave_addr;
			save_channel = intf->target_channel;
			intf->target_channel = fru->channel_num;
		}
		/* print FRU */
		rc = __ipmi_fru_print(intf, fru->device_id);
		if (bridged_request) {
			intf->target_addr = save_addr;
			intf->target_channel = save_channel;
		}
		break;
	case 0x01:
		rc = ipmi_spd_print_fru(intf, fru->device_id);
		break;
	default:
		if (verbose)
			printf(" Unsupported device 0x%02x "
					"type 0x%02x with modifier 0x%02x\n",
					fru->device_id, fru->dev_type,
					fru->dev_type_modifier);
		else
			printf(" Unsupported device\n");
	}
	printf("\n");

	return rc;
}