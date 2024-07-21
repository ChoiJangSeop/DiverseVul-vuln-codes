void Process_v9(void *in_buff, ssize_t in_buff_cnt, FlowSource_t *fs) {
exporter_v9_domain_t	*exporter;
void				*flowset_header;
option_template_flowset_t	*option_flowset;
netflow_v9_header_t	*v9_header;
int64_t 			distance;
uint32_t 			flowset_id, flowset_length, exporter_id;
ssize_t				size_left;
static int pkg_num = 0;

	pkg_num++;
	size_left = in_buff_cnt;
	if ( size_left < NETFLOW_V9_HEADER_LENGTH ) {
		syslog(LOG_ERR, "Process_v9: Too little data for v9 packet: '%lli'", (long long)size_left);
		return;
	}

	// map v9 data structure to input buffer
	v9_header 	= (netflow_v9_header_t *)in_buff;
	exporter_id = ntohl(v9_header->source_id);

	exporter	= GetExporter(fs, exporter_id);
	if ( !exporter ) {
		syslog(LOG_ERR,"Process_v9: Exporter NULL: Abort v9 record processing");
		return;
	}
	exporter->packets++;

	/* calculate boot time in msec */
  	v9_header->SysUptime 	= ntohl(v9_header->SysUptime);
  	v9_header->unix_secs	= ntohl(v9_header->unix_secs);
	exporter->boot_time  	= (uint64_t)1000 * (uint64_t)(v9_header->unix_secs) - (uint64_t)v9_header->SysUptime;
	
	flowset_header 			= (void *)v9_header + NETFLOW_V9_HEADER_LENGTH;

	size_left -= NETFLOW_V9_HEADER_LENGTH;

#ifdef DEVEL
	uint32_t expected_records 		= ntohs(v9_header->count);
	printf("\n[%u] Next packet: %i %u records, buffer: %li \n", exporter_id, pkg_num, expected_records, size_left);
#endif

	// sequence check
	if ( exporter->first ) {
		exporter->last_sequence = ntohl(v9_header->sequence);
		exporter->sequence 	  	= exporter->last_sequence;
		exporter->first			= 0;
	} else {
		exporter->last_sequence = exporter->sequence;
		exporter->sequence 	  = ntohl(v9_header->sequence);
		distance 	  = exporter->sequence - exporter->last_sequence;
		// handle overflow
		if (distance < 0) {
			distance = 0xffffffff + distance  +1;
		}
		if (distance != 1) {
			exporter->sequence_failure++;
			fs->nffile->stat_record->sequence_failure++;
			dbg_printf("[%u] Sequence error: last seq: %lli, seq %lli dist %lli\n", 
				exporter->info.id, (long long)exporter->last_sequence, (long long)exporter->sequence, (long long)distance);
			/*
			if ( report_seq ) 
				syslog(LOG_ERR,"Flow sequence mismatch. Missing: %lli packets", delta(last_count,distance));
			*/
		}
	}

	processed_records = 0;

	// iterate over all flowsets in export packet, while there are bytes left
	flowset_length = 0;
	while (size_left) {
		flowset_header = flowset_header + flowset_length;

		flowset_id 		= GET_FLOWSET_ID(flowset_header);
		flowset_length 	= GET_FLOWSET_LENGTH(flowset_header);
			
		dbg_printf("[%u] Next flowset: %u, length: %u buffersize: %li addr: %llu\n", 
			exporter->info.id, flowset_id, flowset_length, size_left, 
			(long long unsigned)(flowset_header - in_buff) );

		if ( flowset_length == 0 ) {
			/* 	this should never happen, as 4 is an empty flowset 
				and smaller is an illegal flowset anyway ...
				if it happends, we can't determine the next flowset, so skip the entire export packet
			 */
			syslog(LOG_ERR,"Process_v9: flowset zero length error.");
			dbg_printf("Process_v9: flowset zero length error.\n");
			return;
		}

		// possible padding
		if ( flowset_length <= 4 ) {
			size_left = 0;
			continue;
		}

		if ( flowset_length > size_left ) {
			dbg_printf("flowset length error. Expected bytes: %u > buffersize: %lli", 
				flowset_length, (long long)size_left);
			syslog(LOG_ERR,"Process_v9: flowset length error. Expected bytes: %u > buffersize: %lli", 
				flowset_length, (long long)size_left);
			size_left = 0;
			continue;
		}

#ifdef DEVEL
		if ( (ptrdiff_t)fs->nffile->buff_ptr & 0x3 ) {
			fprintf(stderr, "PANIC: alignment error!! \n");
			exit(255);
		}
#endif

		switch (flowset_id) {
			case NF9_TEMPLATE_FLOWSET_ID:
				Process_v9_templates(exporter, flowset_header, fs);
				break;
			case NF9_OPTIONS_FLOWSET_ID:
				option_flowset = (option_template_flowset_t *)flowset_header;
				syslog(LOG_DEBUG,"Process_v9: Found options flowset: template %u", ntohs(option_flowset->template_id));
				Process_v9_option_templates(exporter, flowset_header, fs);
				break;
			default: {
				input_translation_t *table;
				if ( flowset_id < NF9_MIN_RECORD_FLOWSET_ID ) {
					dbg_printf("Invalid flowset id: %u\n", flowset_id);
					syslog(LOG_ERR,"Process_v9: Invalid flowset id: %u", flowset_id);
				} else {

					dbg_printf("[%u] ID %u Data flowset\n", exporter->info.id, flowset_id);

					table = GetTranslationTable(exporter, flowset_id);
					if ( table ) {
						Process_v9_data(exporter, flowset_header, fs, table);
					} else if ( HasOptionTable(fs, flowset_id) ) {
						Process_v9_option_data(exporter, flowset_header, fs);
					} else {
						// maybe a flowset with option data
						dbg_printf("Process v9: [%u] No table for id %u -> Skip record\n", 
							exporter->info.id, flowset_id);
					}
				}
			}
		}

		// next flowset
		size_left -= flowset_length;

	} // End of while 

#ifdef DEVEL
	if ( processed_records != expected_records ) {
		syslog(LOG_ERR, "Process_v9: Processed records %u, expected %u", processed_records, expected_records);
		printf("Process_v9: Processed records %u, expected %u\n", processed_records, expected_records);
	}
#endif

	return;
	
} /* End of Process_v9 */