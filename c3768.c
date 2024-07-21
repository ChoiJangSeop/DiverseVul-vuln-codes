static void InsertStdSamplerOffset( FlowSource_t *fs, uint16_t id, uint16_t offset_std_sampler_interval, uint16_t offset_std_sampler_algorithm) {
option_offset_t	**t;

	t = &(fs->option_offset_table);
	while ( *t ) {
		if ( (*t)->id == id ) { // table already known to us - update data
			dbg_printf("Found existing std sampling info in template %i\n", id);
			break;
		}
	
		t = &((*t)->next);
	}

	if ( *t == NULL ) {	// new table
		dbg_printf("Allocate new std sampling info from template %i\n", id);
		*t = (option_offset_t *)calloc(1, sizeof(option_offset_t));
		if ( !*t ) {
			fprintf(stderr, "malloc() allocation error: %s\n", strerror(errno));
			return ;
		} 
		syslog(LOG_ERR, "Process_v9: New std sampler: interval: %i, algorithm: %i", 
			offset_std_sampler_interval, offset_std_sampler_algorithm);
	}	// else existing table

	dbg_printf("Insert/Update sampling info from template %i\n", id);
	SetFlag((*t)->flags, HAS_STD_SAMPLER_DATA);
	(*t)->id 				= id;
	(*t)->offset_id			= 0;
	(*t)->offset_mode		= 0;
	(*t)->offset_interval	= 0;
	(*t)->offset_std_sampler_interval	= offset_std_sampler_interval;
	(*t)->offset_std_sampler_algorithm	= offset_std_sampler_algorithm;
	
} // End of InsertStdSamplerOffset