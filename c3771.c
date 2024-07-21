void Process_IPFIX(void *in_buff, ssize_t in_buff_cnt, FlowSource_t *fs) {
exporter_ipfix_domain_t	*exporter;
ssize_t				size_left;
uint32_t			ExportTime, ObservationDomain, Sequence, flowset_length;
ipfix_header_t		*ipfix_header;
void				*flowset_header;
#ifdef DEVEL
static uint32_t		packet_cntr = 0;
#endif

	size_left 	 = in_buff_cnt;
	if ( size_left < IPFIX_HEADER_LENGTH ) {
		syslog(LOG_ERR, "Process_ipfix: Too little data for ipfix packet: '%lli'", (long long)size_left);
		return;
	}

	ipfix_header = (ipfix_header_t *)in_buff;
	ObservationDomain 	 = ntohl(ipfix_header->ObservationDomain);
	ExportTime 			 = ntohl(ipfix_header->ExportTime);
	Sequence 			 = ntohl(ipfix_header->LastSequence);

	exporter	= GetExporter(fs, ipfix_header);
	if ( !exporter ) {
		syslog(LOG_ERR,"Process_ipfix: Exporter NULL: Abort ipfix record processing");
		return;
	}
	exporter->packets++;
	//exporter->PacketSequence = Sequence;
	flowset_header	= (void *)ipfix_header + IPFIX_HEADER_LENGTH;
	size_left 	   -= IPFIX_HEADER_LENGTH;

	dbg_printf("\n[%u] Next packet: %u, exported: %s, TemplateRecords: %llu, DataRecords: %llu, buffer: %li \n", 
		ObservationDomain, packet_cntr++, UNIX2ISO(ExportTime), (long long unsigned)exporter->TemplateRecords, 
		(long long unsigned)exporter->DataRecords, size_left);

	dbg_printf("[%u] Sequence: %u\n", ObservationDomain, Sequence);

	// sequence check
	// 2^32 wrap is handled automatically as both counters overflow
	if ( Sequence != exporter->PacketSequence ) {
		if ( exporter->DataRecords != 0 ) {
			// sync sequence on first data record without error report
			fs->nffile->stat_record->sequence_failure++;
			exporter->sequence_failure++;
			dbg_printf("[%u] Sequence check failed: last seq: %u, seq %u\n", 
				exporter->info.id, Sequence, exporter->PacketSequence);
			/* maybee to noise onbuggy exporters
			syslog(LOG_ERR, "Process_ipfix [%u] Sequence error: last seq: %u, seq %u\n", 
				info.id, exporter->LastSequence, Sequence);
			*/
		} else {
			dbg_printf("[%u] Sync Sequence: %u\n", exporter->info.id, Sequence);
		}
		exporter->PacketSequence = Sequence;
	} else {
		dbg_printf("[%u] Sequence check ok\n", exporter->info.id);
	}

	// iterate over all set
	flowset_length = 0;
	while (size_left) {
		uint16_t	flowset_id;

		flowset_header = flowset_header + flowset_length;

		flowset_id 		= GET_FLOWSET_ID(flowset_header);
		flowset_length 	= GET_FLOWSET_LENGTH(flowset_header);

		dbg_printf("Process_ipfix: Next flowset %u, length %u.\n", flowset_id, flowset_length);

		if ( flowset_length == 0 ) {
			/* 	this should never happen, as 4 is an empty flowset 
				and smaller is an illegal flowset anyway ...
				if it happends, we can't determine the next flowset, so skip the entire export packet
			 */
			syslog(LOG_ERR,"Process_ipfix: flowset zero length error.");
			dbg_printf("Process_ipfix: flowset zero length error.\n");
			return;

		}

		// possible padding
		if ( flowset_length <= 4 ) {
			size_left = 0;
			continue;
		}

		if ( flowset_length > size_left ) {
			syslog(LOG_ERR,"Process_ipfix: flowset length error. Expected bytes: %u > buffersize: %lli", 
				flowset_length, (long long)size_left);
			size_left = 0;
			continue;
		}


		switch (flowset_id) {
			case IPFIX_TEMPLATE_FLOWSET_ID:
				// Process_ipfix_templates(exporter, flowset_header, fs);
				exporter->TemplateRecords++;
				dbg_printf("Process template flowset, length: %u\n", flowset_length);
				Process_ipfix_templates(exporter, flowset_header, flowset_length, fs);
				break;
			case IPFIX_OPTIONS_FLOWSET_ID:
				// option_flowset = (option_template_flowset_t *)flowset_header;
				exporter->TemplateRecords++;
				dbg_printf("Process option template flowset, length: %u\n", flowset_length);
				Process_ipfix_option_templates(exporter, flowset_header, fs);
				break;
			default: {
				if ( flowset_id < IPFIX_MIN_RECORD_FLOWSET_ID ) {
					dbg_printf("Invalid flowset id: %u. Skip flowset\n", flowset_id);
					syslog(LOG_ERR,"Process_ipfix: Invalid flowset id: %u. Skip flowset", flowset_id);
				} else {
					input_translation_t *table;
					dbg_printf("Process data flowset, length: %u\n", flowset_length);
					table = GetTranslationTable(exporter, flowset_id);
					if ( table ) {
						Process_ipfix_data(exporter, flowset_header, fs, table);
						exporter->DataRecords++;
					} else if ( HasOptionTable(fs, flowset_id) ) {
						// Process_ipfix_option_data(exporter, flowset_header, fs);
					} else {
						// maybe a flowset with option data
						dbg_printf("Process ipfix: [%u] No table for id %u -> Skip record\n", 
							exporter->info.id, flowset_id);
					}

				}
			}
		} // End of switch

		// next record
		size_left -= flowset_length;

	} // End of while

} // End of Process_IPFIX