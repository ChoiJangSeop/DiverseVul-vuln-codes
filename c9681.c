void __init rtas_initialize(void)
{
	unsigned long rtas_region = RTAS_INSTANTIATE_MAX;
	u32 base, size, entry;
	int no_base, no_size, no_entry;

	/* Get RTAS dev node and fill up our "rtas" structure with infos
	 * about it.
	 */
	rtas.dev = of_find_node_by_name(NULL, "rtas");
	if (!rtas.dev)
		return;

	no_base = of_property_read_u32(rtas.dev, "linux,rtas-base", &base);
	no_size = of_property_read_u32(rtas.dev, "rtas-size", &size);
	if (no_base || no_size) {
		of_node_put(rtas.dev);
		rtas.dev = NULL;
		return;
	}

	rtas.base = base;
	rtas.size = size;
	no_entry = of_property_read_u32(rtas.dev, "linux,rtas-entry", &entry);
	rtas.entry = no_entry ? rtas.base : entry;

	/* If RTAS was found, allocate the RMO buffer for it and look for
	 * the stop-self token if any
	 */
#ifdef CONFIG_PPC64
	if (firmware_has_feature(FW_FEATURE_LPAR)) {
		rtas_region = min(ppc64_rma_size, RTAS_INSTANTIATE_MAX);
		ibm_suspend_me_token = rtas_token("ibm,suspend-me");
	}
#endif
	rtas_rmo_buf = memblock_phys_alloc_range(RTAS_RMOBUF_MAX, PAGE_SIZE,
						 0, rtas_region);
	if (!rtas_rmo_buf)
		panic("ERROR: RTAS: Failed to allocate %lx bytes below %pa\n",
		      PAGE_SIZE, &rtas_region);

#ifdef CONFIG_RTAS_ERROR_LOGGING
	rtas_last_error_token = rtas_token("rtas-last-error");
#endif
}