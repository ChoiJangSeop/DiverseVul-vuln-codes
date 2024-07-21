ipmi_sdr_print_sensor_eventonly(struct ipmi_intf *intf,
				struct sdr_record_eventonly_sensor *sensor)
{
	char desc[17];

	if (!sensor)
		return -1;

	memset(desc, 0, sizeof (desc));
	snprintf(desc, (sensor->id_code & 0x1f) + 1, "%s", sensor->id_string);

	if (verbose) {
		printf("Sensor ID              : %s (0x%x)\n",
		       sensor->id_code ? desc : "", sensor->keys.sensor_num);
		printf("Entity ID              : %d.%d (%s)\n",
		       sensor->entity.id, sensor->entity.instance,
		       val2str(sensor->entity.id, entity_id_vals));
		printf("Sensor Type            : %s (0x%02x)\n",
			ipmi_get_sensor_type(intf, sensor->sensor_type),
			sensor->sensor_type);
		lprintf(LOG_DEBUG, "Event Type Code        : 0x%02x",
			sensor->event_type);
		printf("\n");
	} else {
		if (csv_output)
			printf("%s,%02Xh,ns,%d.%d,Event-Only\n",
			       sensor->id_code ? desc : "",
			       sensor->keys.sensor_num,
			       sensor->entity.id, sensor->entity.instance);
		else if (sdr_extended)
			printf("%-16s | %02Xh | ns  | %2d.%1d | Event-Only\n",
			       sensor->id_code ? desc : "",
			       sensor->keys.sensor_num,
			       sensor->entity.id, sensor->entity.instance);
		else
			printf("%-16s | Event-Only        | ns\n",
			       sensor->id_code ? desc : "");
	}

	return 0;
}