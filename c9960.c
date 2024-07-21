static void rtps_util_detect_coherent_set_end_empty_data_case(

  coherent_set_entity_info *coherent_set_entity_info_object) {
  coherent_set_entity_info *coherent_set_entry = NULL;

  coherent_set_entry = (coherent_set_entity_info*) wmem_map_lookup(coherent_set_tracking.entities_using_map, &coherent_set_entity_info_object->guid);
  if (coherent_set_entry) {
    coherent_set_info *coherent_set_info_entry;
    coherent_set_key key;

    key.guid = coherent_set_entity_info_object->guid;
    key.coherent_set_seq_number = coherent_set_entry->coherent_set_seq_number;

    coherent_set_info_entry = (coherent_set_info*)wmem_map_lookup(coherent_set_tracking.coherent_set_registry_map, &key);
    if (coherent_set_info_entry) {
      if (coherent_set_entry->expected_coherent_set_end_writers_seq_number == coherent_set_entity_info_object->writer_seq_number) {

        coherent_set_info_entry->is_set = TRUE;
        coherent_set_info_entry->writer_seq_number = coherent_set_entry->expected_coherent_set_end_writers_seq_number - 1;
      }
    }
  }
}