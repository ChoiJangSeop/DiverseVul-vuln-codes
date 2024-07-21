static void remove_all_translation_tables(exporter_ipfix_domain_t *exporter) {
input_translation_t *table, *next;

	syslog(LOG_INFO, "Process_ipfix: Withdraw all templates from observation domain %u\n", 
		exporter->info.id);

	table = exporter->input_translation_table;
	while ( table ) {
		next = table->next;

		dbg_printf("\n[%u] Withdraw template ID: %u\n", exporter->info.id, table->id);

		free(table->sequence);
		free(table->extension_info.map);
		free(table);

		table = next;
	}

	// clear references
	exporter->input_translation_table = NULL;
	exporter->current_table = NULL;

} // End of remove_all_translation_tables