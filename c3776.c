int Init_v9(void) {
int i;

	output_templates = NULL;

	cache.lookup_info	    = (struct element_param_s *)calloc(65536, sizeof(struct element_param_s));
	cache.common_extensions = (uint32_t *)malloc((Max_num_extensions+1)*sizeof(uint32_t));
	if ( !cache.common_extensions || !cache.lookup_info ) {
		syslog(LOG_ERR, "Process_v9: Panic! malloc(): %s line %d: %s", __FILE__, __LINE__, strerror (errno));
		return 0;
	}

	// init the helper element table
	for (i=1; v9_element_map[i].id != 0; i++ ) {
		uint32_t Type = v9_element_map[i].id;
		// multiple same type - save first index only
		// iterate through same Types afterwards
		if ( cache.lookup_info[Type].index == 0 ) 
			cache.lookup_info[Type].index  = i;
	}
	cache.max_v9_elements = i;

	syslog(LOG_DEBUG,"Init v9: Max number of v9 tags: %u", cache.max_v9_elements);


	return 1;
	
} // End of Init_v9