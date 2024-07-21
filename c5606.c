static bool write_log_dropped_partitions(ALTER_PARTITION_PARAM_TYPE *lpt,
                                         uint *next_entry,
                                         const char *path,
                                         bool temp_list)
{
  DDL_LOG_ENTRY ddl_log_entry;
  partition_info *part_info= lpt->part_info;
  DDL_LOG_MEMORY_ENTRY *log_entry;
  char tmp_path[FN_LEN];
  List_iterator<partition_element> part_it(part_info->partitions);
  List_iterator<partition_element> temp_it(part_info->temp_partitions);
  uint num_temp_partitions= part_info->temp_partitions.elements;
  uint num_elements= part_info->partitions.elements;
  DBUG_ENTER("write_log_dropped_partitions");

  ddl_log_entry.action_type= DDL_LOG_DELETE_ACTION;
  if (temp_list)
    num_elements= num_temp_partitions;
  while (num_elements--)
  {
    partition_element *part_elem;
    if (temp_list)
      part_elem= temp_it++;
    else
      part_elem= part_it++;
    if (part_elem->part_state == PART_TO_BE_DROPPED ||
        part_elem->part_state == PART_TO_BE_ADDED ||
        part_elem->part_state == PART_CHANGED)
    {
      uint name_variant;
      if (part_elem->part_state == PART_CHANGED ||
          (part_elem->part_state == PART_TO_BE_ADDED &&
           num_temp_partitions))
        name_variant= TEMP_PART_NAME;
      else
        name_variant= NORMAL_PART_NAME;
      if (part_info->is_sub_partitioned())
      {
        List_iterator<partition_element> sub_it(part_elem->subpartitions);
        uint num_subparts= part_info->num_subparts;
        uint j= 0;
        do
        {
          partition_element *sub_elem= sub_it++;
          ddl_log_entry.next_entry= *next_entry;
          ddl_log_entry.handler_name=
               ha_resolve_storage_engine_name(sub_elem->engine_type);
          create_subpartition_name(tmp_path, path,
                                   part_elem->partition_name,
                                   sub_elem->partition_name,
                                   name_variant);
          ddl_log_entry.name= tmp_path;
          if (write_ddl_log_entry(&ddl_log_entry, &log_entry))
          {
            DBUG_RETURN(TRUE);
          }
          *next_entry= log_entry->entry_pos;
          sub_elem->log_entry= log_entry;
          insert_part_info_log_entry_list(part_info, log_entry);
        } while (++j < num_subparts);
      }
      else
      {
        ddl_log_entry.next_entry= *next_entry;
        ddl_log_entry.handler_name=
               ha_resolve_storage_engine_name(part_elem->engine_type);
        create_partition_name(tmp_path, path,
                              part_elem->partition_name,
                              name_variant, TRUE);
        ddl_log_entry.name= tmp_path;
        if (write_ddl_log_entry(&ddl_log_entry, &log_entry))
        {
          DBUG_RETURN(TRUE);
        }
        *next_entry= log_entry->entry_pos;
        part_elem->log_entry= log_entry;
        insert_part_info_log_entry_list(part_info, log_entry);
      }
    }
  }
  DBUG_RETURN(FALSE);
}