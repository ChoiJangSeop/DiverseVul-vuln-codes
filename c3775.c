static void remove_translation_table(FlowSource_t *fs, exporter_ipfix_domain_t *exporter, uint16_t id) {
input_translation_t *table, *parent;

	syslog(LOG_INFO, "Process_ipfix: [%u] Withdraw template id: %i", 
			exporter->info.id, id);

	parent = NULL;
	table = exporter->input_translation_table;
	while ( table && ( table->id != id ) ) {
		parent = table;
		table = table->next;
	}

	if ( table == NULL ) {
		syslog(LOG_ERR, "Process_ipfix: [%u] Withdraw template id: %i. translation table not found", 
				exporter->info.id, id);
		return;
	}

	dbg_printf("\n[%u] Withdraw template ID: %u\n", exporter->info.id, table->id);

	// clear table cache, if this is the table to delete
	if (exporter->current_table == table)
		exporter->current_table = NULL;

	if ( parent ) {
		// remove table from list
		parent->next = table->next;
	} else {
		// last table removed
		exporter->input_translation_table = NULL;
	}

	RemoveExtensionMap(fs, table->extension_info.map);
	free(table->sequence);
	free(table->extension_info.map);
	free(table);

} // End of remove_translation_table