static void create_power_zone_common_attributes(
					struct powercap_zone *power_zone)
{
	int count = 0;

	power_zone->zone_dev_attrs[count++] = &dev_attr_name.attr;
	if (power_zone->ops->get_max_energy_range_uj)
		power_zone->zone_dev_attrs[count++] =
					&dev_attr_max_energy_range_uj.attr;
	if (power_zone->ops->get_energy_uj) {
		if (power_zone->ops->reset_energy_uj)
			dev_attr_energy_uj.attr.mode = S_IWUSR | S_IRUGO;
		else
			dev_attr_energy_uj.attr.mode = S_IRUGO;
		power_zone->zone_dev_attrs[count++] =
					&dev_attr_energy_uj.attr;
	}
	if (power_zone->ops->get_power_uw)
		power_zone->zone_dev_attrs[count++] =
					&dev_attr_power_uw.attr;
	if (power_zone->ops->get_max_power_range_uw)
		power_zone->zone_dev_attrs[count++] =
					&dev_attr_max_power_range_uw.attr;
	power_zone->zone_dev_attrs[count] = NULL;
	power_zone->zone_attr_count = count;
}