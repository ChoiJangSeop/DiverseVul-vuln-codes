static void rtps_util_add_coherent_set_general_cases_case(
  proto_tree *tree,
  tvbuff_t *tvb,
  guint64 coherent_seq_number,
  coherent_set_entity_info *coherent_set_entity_info_object) {

  coherent_set_entity_info *register_entry;
  proto_tree *marked_item_tree;
  coherent_set_info *coherent_set_info_entry;
  coherent_set_key coherent_set_info_key;

  coherent_set_entity_info_object->coherent_set_seq_number = coherent_seq_number;
  register_entry = (coherent_set_entity_info*)wmem_map_lookup(coherent_set_tracking.entities_using_map,
    &coherent_set_entity_info_object->guid);
  if (!register_entry) {
    register_entry = (coherent_set_entity_info*)wmem_memdup(wmem_file_scope(), coherent_set_entity_info_object, sizeof(coherent_set_entity_info));
    wmem_map_insert(
      coherent_set_tracking.entities_using_map,
      &register_entry->guid,
      register_entry);
  }

  coherent_set_info_key.guid = coherent_set_entity_info_object->guid;
  coherent_set_info_key.coherent_set_seq_number = coherent_seq_number;
  coherent_set_info_entry = (coherent_set_info*)wmem_map_lookup(coherent_set_tracking.coherent_set_registry_map,
    &coherent_set_info_key);
  if (!coherent_set_info_entry) {
    coherent_set_info_entry = wmem_new0(wmem_file_scope(), coherent_set_info);
    coherent_set_info_entry->key = (coherent_set_key*)wmem_memdup(wmem_file_scope(), &coherent_set_info_key, sizeof(coherent_set_key));
    coherent_set_info_entry->is_set = FALSE;
    wmem_map_insert(
      coherent_set_tracking.coherent_set_registry_map,
      coherent_set_info_entry->key,
      coherent_set_info_entry);
  }

  if (coherent_set_info_entry->writer_seq_number < coherent_set_entity_info_object->writer_seq_number) {
    coherent_set_info_entry->writer_seq_number = coherent_set_entity_info_object->writer_seq_number;
  }
  /* Start */
  if (coherent_set_entity_info_object->coherent_set_seq_number == coherent_set_entity_info_object->writer_seq_number) {
    marked_item_tree = proto_tree_add_uint64(tree, hf_rtps_coherent_set_start,
      tvb, 0, 0, coherent_seq_number);
    proto_item_set_generated(marked_item_tree);

    /* End case: Start of a new coherent set */
    if (coherent_set_entity_info_object->coherent_set_seq_number > register_entry->coherent_set_seq_number &&
      coherent_set_entity_info_object->writer_seq_number - 1 == register_entry->writer_seq_number) {
      coherent_set_info *previous_entry;

      marked_item_tree = proto_tree_add_uint64(tree, hf_rtps_coherent_set_end,
        tvb, 0, 0, register_entry->coherent_set_seq_number);
      proto_item_set_generated(marked_item_tree);
      coherent_set_info_key.coherent_set_seq_number = register_entry->writer_seq_number;
      coherent_set_info_key.guid = register_entry->guid;
      previous_entry = (coherent_set_info*)wmem_map_lookup(coherent_set_tracking.coherent_set_registry_map, &coherent_set_info_key);
      if (previous_entry) {
        previous_entry->is_set = TRUE;
      }
    }
  }

  if (!coherent_set_info_entry->is_set) {

    coherent_set_info_key.coherent_set_seq_number = coherent_seq_number - 1;

    /* End case: Sequence unknown received */
    if (coherent_set_entity_info_object->coherent_set_seq_number == RTPS_SEQUENCENUMBER_UNKNOWN) {
      register_entry->coherent_set_seq_number = coherent_set_entity_info_object->coherent_set_seq_number;
      marked_item_tree = proto_tree_add_uint64(tree, hf_rtps_coherent_set_end,
        tvb, 0, 0, coherent_set_info_entry->key->coherent_set_seq_number);
      proto_item_set_generated(marked_item_tree);
      coherent_set_info_entry->is_set = TRUE;
    }
  } else if (coherent_set_info_entry->writer_seq_number == coherent_set_entity_info_object->writer_seq_number) {
    proto_tree *ti;

    ti = proto_tree_add_uint64(tree, hf_rtps_coherent_set_end,
      tvb, 0, 0, coherent_set_info_entry->key->coherent_set_seq_number);
    proto_item_set_generated(ti);
  }
  /* Update the entity */
  coherent_set_entity_info_object->expected_coherent_set_end_writers_seq_number = coherent_set_entity_info_object->writer_seq_number + 1;
  *register_entry = *coherent_set_entity_info_object;
}