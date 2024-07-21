int ha_partition::change_partitions(HA_CREATE_INFO *create_info,
                                    const char *path,
                                    ulonglong * const copied,
                                    ulonglong * const deleted,
                                    const uchar *pack_frm_data
                                    __attribute__((unused)),
                                    size_t pack_frm_len
                                    __attribute__((unused)))
{
  List_iterator<partition_element> part_it(m_part_info->partitions);
  List_iterator <partition_element> t_it(m_part_info->temp_partitions);
  char part_name_buff[FN_REFLEN];
  uint num_parts= m_part_info->partitions.elements;
  uint num_subparts= m_part_info->num_subparts;
  uint i= 0;
  uint num_remain_partitions, part_count, orig_count;
  handler **new_file_array;
  int error= 1;
  bool first;
  uint temp_partitions= m_part_info->temp_partitions.elements;
  THD *thd= ha_thd();
  DBUG_ENTER("ha_partition::change_partitions");

  /*
    Assert that it works without HA_FILE_BASED and lower_case_table_name = 2.
    We use m_file[0] as long as all partitions have the same storage engine.
  */
  DBUG_ASSERT(!strcmp(path, get_canonical_filename(m_file[0], path,
                                                   part_name_buff)));
  m_reorged_parts= 0;
  if (!m_part_info->is_sub_partitioned())
    num_subparts= 1;

  /*
    Step 1:
      Calculate number of reorganised partitions and allocate space for
      their handler references.
  */
  if (temp_partitions)
  {
    m_reorged_parts= temp_partitions * num_subparts;
  }
  else
  {
    do
    {
      partition_element *part_elem= part_it++;
      if (part_elem->part_state == PART_CHANGED ||
          part_elem->part_state == PART_REORGED_DROPPED)
      {
        m_reorged_parts+= num_subparts;
      }
    } while (++i < num_parts);
  }
  if (m_reorged_parts &&
      !(m_reorged_file= (handler**)sql_calloc(sizeof(handler*)*
                                              (m_reorged_parts + 1))))
  {
    mem_alloc_error(sizeof(handler*)*(m_reorged_parts+1));
    DBUG_RETURN(ER_OUTOFMEMORY);
  }

  /*
    Step 2:
      Calculate number of partitions after change and allocate space for
      their handler references.
  */
  num_remain_partitions= 0;
  if (temp_partitions)
  {
    num_remain_partitions= num_parts * num_subparts;
  }
  else
  {
    part_it.rewind();
    i= 0;
    do
    {
      partition_element *part_elem= part_it++;
      if (part_elem->part_state == PART_NORMAL ||
          part_elem->part_state == PART_TO_BE_ADDED ||
          part_elem->part_state == PART_CHANGED)
      {
        num_remain_partitions+= num_subparts;
      }
    } while (++i < num_parts);
  }
  if (!(new_file_array= (handler**)sql_calloc(sizeof(handler*)*
                                            (2*(num_remain_partitions + 1)))))
  {
    mem_alloc_error(sizeof(handler*)*2*(num_remain_partitions+1));
    DBUG_RETURN(ER_OUTOFMEMORY);
  }
  m_added_file= &new_file_array[num_remain_partitions + 1];

  /*
    Step 3:
      Fill m_reorged_file with handler references and NULL at the end
  */
  if (m_reorged_parts)
  {
    i= 0;
    part_count= 0;
    first= TRUE;
    part_it.rewind();
    do
    {
      partition_element *part_elem= part_it++;
      if (part_elem->part_state == PART_CHANGED ||
          part_elem->part_state == PART_REORGED_DROPPED)
      {
        memcpy((void*)&m_reorged_file[part_count],
               (void*)&m_file[i*num_subparts],
               sizeof(handler*)*num_subparts);
        part_count+= num_subparts;
      }
      else if (first && temp_partitions &&
               part_elem->part_state == PART_TO_BE_ADDED)
      {
        /*
          When doing an ALTER TABLE REORGANIZE PARTITION a number of
          partitions is to be reorganised into a set of new partitions.
          The reorganised partitions are in this case in the temp_partitions
          list. We copy all of them in one batch and thus we only do this
          until we find the first partition with state PART_TO_BE_ADDED
          since this is where the new partitions go in and where the old
          ones used to be.
        */
        first= FALSE;
        DBUG_ASSERT(((i*num_subparts) + m_reorged_parts) <= m_file_tot_parts);
        memcpy((void*)m_reorged_file, &m_file[i*num_subparts],
               sizeof(handler*)*m_reorged_parts);
      }
    } while (++i < num_parts);
  }

  /*
    Step 4:
      Fill new_array_file with handler references. Create the handlers if
      needed.
  */
  i= 0;
  part_count= 0;
  orig_count= 0;
  first= TRUE;
  part_it.rewind();
  do
  {
    partition_element *part_elem= part_it++;
    if (part_elem->part_state == PART_NORMAL)
    {
      DBUG_ASSERT(orig_count + num_subparts <= m_file_tot_parts);
      memcpy((void*)&new_file_array[part_count], (void*)&m_file[orig_count],
             sizeof(handler*)*num_subparts);
      part_count+= num_subparts;
      orig_count+= num_subparts;
    }
    else if (part_elem->part_state == PART_CHANGED ||
             part_elem->part_state == PART_TO_BE_ADDED)
    {
      uint j= 0;
      do
      {
        if (!(new_file_array[part_count++]=
              get_new_handler(table->s,
                              thd->mem_root,
                              part_elem->engine_type)))
        {
          mem_alloc_error(sizeof(handler));
          DBUG_RETURN(ER_OUTOFMEMORY);
        }
      } while (++j < num_subparts);
      if (part_elem->part_state == PART_CHANGED)
        orig_count+= num_subparts;
      else if (temp_partitions && first)
      {
        orig_count+= (num_subparts * temp_partitions);
        first= FALSE;
      }
    }
  } while (++i < num_parts);
  first= FALSE;
  /*
    Step 5:
      Create the new partitions and also open, lock and call external_lock
      on them to prepare them for copy phase and also for later close
      calls
  */

  /*
     Before creating new partitions check whether indexes are disabled
     in the  partitions.
  */

  uint disable_non_uniq_indexes = indexes_are_disabled();

  i= 0;
  part_count= 0;
  part_it.rewind();
  do
  {
    partition_element *part_elem= part_it++;
    if (part_elem->part_state == PART_TO_BE_ADDED ||
        part_elem->part_state == PART_CHANGED)
    {
      /*
        A new partition needs to be created PART_TO_BE_ADDED means an
        entirely new partition and PART_CHANGED means a changed partition
        that will still exist with either more or less data in it.
      */
      uint name_variant= NORMAL_PART_NAME;
      if (part_elem->part_state == PART_CHANGED ||
          (part_elem->part_state == PART_TO_BE_ADDED && temp_partitions))
        name_variant= TEMP_PART_NAME;
      if (m_part_info->is_sub_partitioned())
      {
        List_iterator<partition_element> sub_it(part_elem->subpartitions);
        uint j= 0, part;
        do
        {
          partition_element *sub_elem= sub_it++;
          create_subpartition_name(part_name_buff, path,
                                   part_elem->partition_name,
                                   sub_elem->partition_name,
                                   name_variant);
          part= i * num_subparts + j;
          DBUG_PRINT("info", ("Add subpartition %s", part_name_buff));
          if ((error= prepare_new_partition(table, create_info,
                                            new_file_array[part],
                                            (const char *)part_name_buff,
                                            sub_elem,
                                            disable_non_uniq_indexes)))
          {
            cleanup_new_partition(part_count);
            DBUG_RETURN(error);
          }

          m_added_file[part_count++]= new_file_array[part];
        } while (++j < num_subparts);
      }
      else
      {
        create_partition_name(part_name_buff, path,
                              part_elem->partition_name, name_variant,
                              TRUE);
        DBUG_PRINT("info", ("Add partition %s", part_name_buff));
        if ((error= prepare_new_partition(table, create_info,
                                          new_file_array[i],
                                          (const char *)part_name_buff,
                                          part_elem,
                                          disable_non_uniq_indexes)))
        {
          cleanup_new_partition(part_count);
          DBUG_RETURN(error);
        }

        m_added_file[part_count++]= new_file_array[i];
      }
    }
  } while (++i < num_parts);

  /*
    Step 6:
      State update to prepare for next write of the frm file.
  */
  i= 0;
  part_it.rewind();
  do
  {
    partition_element *part_elem= part_it++;
    if (part_elem->part_state == PART_TO_BE_ADDED)
      part_elem->part_state= PART_IS_ADDED;
    else if (part_elem->part_state == PART_CHANGED)
      part_elem->part_state= PART_IS_CHANGED;
    else if (part_elem->part_state == PART_REORGED_DROPPED)
      part_elem->part_state= PART_TO_BE_DROPPED;
  } while (++i < num_parts);
  for (i= 0; i < temp_partitions; i++)
  {
    partition_element *part_elem= t_it++;
    DBUG_ASSERT(part_elem->part_state == PART_TO_BE_REORGED);
    part_elem->part_state= PART_TO_BE_DROPPED;
  }
  m_new_file= new_file_array;
  if ((error= copy_partitions(copied, deleted)))
  {
    /*
      Close and unlock the new temporary partitions.
      They will later be deleted through the ddl-log.
    */
    cleanup_new_partition(part_count);
  }
  DBUG_RETURN(error);
}