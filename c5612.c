int ha_partition::rename_partitions(const char *path)
{
  List_iterator<partition_element> part_it(m_part_info->partitions);
  List_iterator<partition_element> temp_it(m_part_info->temp_partitions);
  char part_name_buff[FN_REFLEN];
  char norm_name_buff[FN_REFLEN];
  uint num_parts= m_part_info->partitions.elements;
  uint part_count= 0;
  uint num_subparts= m_part_info->num_subparts;
  uint i= 0;
  uint j= 0;
  int error= 0;
  int ret_error;
  uint temp_partitions= m_part_info->temp_partitions.elements;
  handler *file;
  partition_element *part_elem, *sub_elem;
  DBUG_ENTER("ha_partition::rename_partitions");

  /*
    Assert that it works without HA_FILE_BASED and lower_case_table_name = 2.
    We use m_file[0] as long as all partitions have the same storage engine.
  */
  DBUG_ASSERT(!strcmp(path, get_canonical_filename(m_file[0], path,
                                                   norm_name_buff)));

  DEBUG_SYNC(ha_thd(), "before_rename_partitions");
  if (temp_partitions)
  {
    /*
      These are the reorganised partitions that have already been copied.
      We delete the partitions and log the delete by inactivating the
      delete log entry in the table log. We only need to synchronise
      these writes before moving to the next loop since there is no
      interaction among reorganised partitions, they cannot have the
      same name.
    */
    do
    {
      part_elem= temp_it++;
      if (m_is_sub_partitioned)
      {
        List_iterator<partition_element> sub_it(part_elem->subpartitions);
        j= 0;
        do
        {
          sub_elem= sub_it++;
          file= m_reorged_file[part_count++];
          create_subpartition_name(norm_name_buff, path,
                                   part_elem->partition_name,
                                   sub_elem->partition_name,
                                   NORMAL_PART_NAME);
          DBUG_PRINT("info", ("Delete subpartition %s", norm_name_buff));
          if ((ret_error= file->ha_delete_table(norm_name_buff)))
            error= ret_error;
          else if (deactivate_ddl_log_entry(sub_elem->log_entry->entry_pos))
            error= 1;
          else
            sub_elem->log_entry= NULL; /* Indicate success */
        } while (++j < num_subparts);
      }
      else
      {
        file= m_reorged_file[part_count++];
        create_partition_name(norm_name_buff, path,
                              part_elem->partition_name, NORMAL_PART_NAME,
                              TRUE);
        DBUG_PRINT("info", ("Delete partition %s", norm_name_buff));
        if ((ret_error= file->ha_delete_table(norm_name_buff)))
          error= ret_error;
        else if (deactivate_ddl_log_entry(part_elem->log_entry->entry_pos))
          error= 1;
        else
          part_elem->log_entry= NULL; /* Indicate success */
      }
    } while (++i < temp_partitions);
    (void) sync_ddl_log();
  }
  i= 0;
  do
  {
    /*
       When state is PART_IS_CHANGED it means that we have created a new
       TEMP partition that is to be renamed to normal partition name and
       we are to delete the old partition with currently the normal name.
       
       We perform this operation by
       1) Delete old partition with normal partition name
       2) Signal this in table log entry
       3) Synch table log to ensure we have consistency in crashes
       4) Rename temporary partition name to normal partition name
       5) Signal this to table log entry
       It is not necessary to synch the last state since a new rename
       should not corrupt things if there was no temporary partition.

       The only other parts we need to cater for are new parts that
       replace reorganised parts. The reorganised parts were deleted
       by the code above that goes through the temp_partitions list.
       Thus the synch above makes it safe to simply perform step 4 and 5
       for those entries.
    */
    part_elem= part_it++;
    if (part_elem->part_state == PART_IS_CHANGED ||
        part_elem->part_state == PART_TO_BE_DROPPED ||
        (part_elem->part_state == PART_IS_ADDED && temp_partitions))
    {
      if (m_is_sub_partitioned)
      {
        List_iterator<partition_element> sub_it(part_elem->subpartitions);
        uint part;

        j= 0;
        do
        {
          sub_elem= sub_it++;
          part= i * num_subparts + j;
          create_subpartition_name(norm_name_buff, path,
                                   part_elem->partition_name,
                                   sub_elem->partition_name,
                                   NORMAL_PART_NAME);
          if (part_elem->part_state == PART_IS_CHANGED)
          {
            file= m_reorged_file[part_count++];
            DBUG_PRINT("info", ("Delete subpartition %s", norm_name_buff));
            if ((ret_error= file->ha_delete_table(norm_name_buff)))
              error= ret_error;
            else if (deactivate_ddl_log_entry(sub_elem->log_entry->entry_pos))
              error= 1;
            (void) sync_ddl_log();
          }
          file= m_new_file[part];
          create_subpartition_name(part_name_buff, path,
                                   part_elem->partition_name,
                                   sub_elem->partition_name,
                                   TEMP_PART_NAME);
          DBUG_PRINT("info", ("Rename subpartition from %s to %s",
                     part_name_buff, norm_name_buff));
          if ((ret_error= file->ha_rename_table(part_name_buff,
                                                norm_name_buff)))
            error= ret_error;
          else if (deactivate_ddl_log_entry(sub_elem->log_entry->entry_pos))
            error= 1;
          else
            sub_elem->log_entry= NULL;
        } while (++j < num_subparts);
      }
      else
      {
        create_partition_name(norm_name_buff, path,
                              part_elem->partition_name, NORMAL_PART_NAME,
                              TRUE);
        if (part_elem->part_state == PART_IS_CHANGED)
        {
          file= m_reorged_file[part_count++];
          DBUG_PRINT("info", ("Delete partition %s", norm_name_buff));
          if ((ret_error= file->ha_delete_table(norm_name_buff)))
            error= ret_error;
          else if (deactivate_ddl_log_entry(part_elem->log_entry->entry_pos))
            error= 1;
          (void) sync_ddl_log();
        }
        file= m_new_file[i];
        create_partition_name(part_name_buff, path,
                              part_elem->partition_name, TEMP_PART_NAME,
                              TRUE);
        DBUG_PRINT("info", ("Rename partition from %s to %s",
                   part_name_buff, norm_name_buff));
        if ((ret_error= file->ha_rename_table(part_name_buff,
                                              norm_name_buff)))
          error= ret_error;
        else if (deactivate_ddl_log_entry(part_elem->log_entry->entry_pos))
          error= 1;
        else
          part_elem->log_entry= NULL;
      }
    }
  } while (++i < num_parts);
  (void) sync_ddl_log();
  DBUG_RETURN(error);
}