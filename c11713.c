static ssize_t valid_zones_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct memory_block *mem = to_memory_block(dev);
	unsigned long start_pfn = section_nr_to_pfn(mem->start_section_nr);
	unsigned long nr_pages = PAGES_PER_SECTION * sections_per_block;
	struct zone *default_zone;
	int nid;

	/*
	 * Check the existing zone. Make sure that we do that only on the
	 * online nodes otherwise the page_zone is not reliable
	 */
	if (mem->state == MEM_ONLINE) {
		/*
		 * The block contains more than one zone can not be offlined.
		 * This can happen e.g. for ZONE_DMA and ZONE_DMA32
		 */
		default_zone = test_pages_in_a_zone(start_pfn,
						    start_pfn + nr_pages);
		if (!default_zone)
			return sprintf(buf, "none\n");
		strcat(buf, default_zone->name);
		goto out;
	}

	nid = mem->nid;
	default_zone = zone_for_pfn_range(MMOP_ONLINE, nid, start_pfn,
					  nr_pages);
	strcat(buf, default_zone->name);

	print_allowed_zone(buf, nid, start_pfn, nr_pages, MMOP_ONLINE_KERNEL,
			default_zone);
	print_allowed_zone(buf, nid, start_pfn, nr_pages, MMOP_ONLINE_MOVABLE,
			default_zone);
out:
	strcat(buf, "\n");

	return strlen(buf);
}