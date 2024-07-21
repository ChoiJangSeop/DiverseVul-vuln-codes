static bool write_log_changed_partitions(ALTER_PARTITION_PARAM_TYPE *lpt,
                                         uint *next_entry, const char *path)
{
  DDL_LOG_ENTRY ddl_log_entry;
  partition_info *part_info= lpt->part_info;
  DDL_LOG_MEMORY_ENTRY *log_entry;
  char tmp_path[FN_REFLEN];
  char normal_path[FN_REFLEN];
  List_iterator<partition_element> part_it(part_info->partitions);
  uint temp_partitions= part_info->temp_partitions.elements;
  uint num_elements= part_info->partitions.elements;
  uint i= 0;
  DBUG_ENTER("write_log_changed_partitions");

  do
  {
    partition_element *part_elem= part_it++;
    if (part_elem->part_state == PART_IS_CHANGED ||
        (part_elem->part_state == PART_IS_ADDED && temp_partitions))
    {
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
                                   TEMP_PART_NAME);
          create_subpartition_name(normal_path, path,
                                   part_elem->partition_name,
                                   sub_elem->partition_name,
                                   NORMAL_PART_NAME);
          ddl_log_entry.name= normal_path;
          ddl_log_entry.from_name= tmp_path;
          if (part_elem->part_state == PART_IS_CHANGED)
            ddl_log_entry.action_type= DDL_LOG_REPLACE_ACTION;
          else
            ddl_log_entry.action_type= DDL_LOG_RENAME_ACTION;
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
                              TEMP_PART_NAME, TRUE);
        create_partition_name(normal_path, path,
                              part_elem->partition_name,
                              NORMAL_PART_NAME, TRUE);
        ddl_log_entry.name= normal_path;
        ddl_log_entry.from_name= tmp_path;
        if (part_elem->part_state == PART_IS_CHANGED)
          ddl_log_entry.action_type= DDL_LOG_REPLACE_ACTION;
        else
          ddl_log_entry.action_type= DDL_LOG_RENAME_ACTION;
        if (write_ddl_log_entry(&ddl_log_entry, &log_entry))
        {
          DBUG_RETURN(TRUE);
        }
        *next_entry= log_entry->entry_pos;
        part_elem->log_entry= log_entry;
        insert_part_info_log_entry_list(part_info, log_entry);
      }
    }
  } while (++i < num_elements);
  DBUG_RETURN(FALSE);
}