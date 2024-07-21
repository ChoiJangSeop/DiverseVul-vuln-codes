int ha_partition::open(const char *name, int mode, uint test_if_locked)
{
  char *name_buffer_ptr;
  int error= HA_ERR_INITIALIZATION;
  handler **file;
  char name_buff[FN_REFLEN];
  bool is_not_tmp_table= (table_share->tmp_table == NO_TMP_TABLE);
  ulonglong check_table_flags;
  DBUG_ENTER("ha_partition::open");

  DBUG_ASSERT(table->s == table_share);
  ref_length= 0;
  m_mode= mode;
  m_open_test_lock= test_if_locked;
  m_part_field_array= m_part_info->full_part_field_array;
  if (get_from_handler_file(name, &table->mem_root, test(m_is_clone_of)))
    DBUG_RETURN(error);
  name_buffer_ptr= m_name_buffer_ptr;
  m_start_key.length= 0;
  m_rec0= table->record[0];
  m_rec_length= table_share->stored_rec_length;
  if (!m_part_ids_sorted_by_num_of_records)
  {
    if (!(m_part_ids_sorted_by_num_of_records=
            (uint32*) my_malloc(m_tot_parts * sizeof(uint32), MYF(MY_WME))))
      DBUG_RETURN(error);
    uint32 i;
    /* Initialize it with all partition ids. */
    for (i= 0; i < m_tot_parts; i++)
      m_part_ids_sorted_by_num_of_records[i]= i;
  }

  /* Initialize the bitmap we use to minimize ha_start_bulk_insert calls */
  if (bitmap_init(&m_bulk_insert_started, NULL, m_tot_parts + 1, FALSE))
    DBUG_RETURN(error);
  bitmap_clear_all(&m_bulk_insert_started);
  /*
    Initialize the bitmap we use to keep track of partitions which returned
    HA_ERR_KEY_NOT_FOUND from index_read_map.
  */
  if (bitmap_init(&m_key_not_found_partitions, NULL, m_tot_parts, FALSE))
  {
    bitmap_free(&m_bulk_insert_started);
    DBUG_RETURN(error);
  }
  bitmap_clear_all(&m_key_not_found_partitions);
  m_key_not_found= false;
  /* Initialize the bitmap we use to determine what partitions are used */
  if (!m_is_clone_of)
  {
    DBUG_ASSERT(!m_clone_mem_root);
    if (bitmap_init(&(m_part_info->used_partitions), NULL, m_tot_parts, TRUE))
    {
      bitmap_free(&m_bulk_insert_started);
      DBUG_RETURN(error);
    }
    bitmap_set_all(&(m_part_info->used_partitions));
  }

  if (m_is_clone_of)
  {
    uint i, alloc_len;
    DBUG_ASSERT(m_clone_mem_root);
    /* Allocate an array of handler pointers for the partitions handlers. */
    alloc_len= (m_tot_parts + 1) * sizeof(handler*);
    if (!(m_file= (handler **) alloc_root(m_clone_mem_root, alloc_len)))
      goto err_alloc;
    memset(m_file, 0, alloc_len);
    /*
      Populate them by cloning the original partitions. This also opens them.
      Note that file->ref is allocated too.
    */
    file= m_is_clone_of->m_file;
    for (i= 0; i < m_tot_parts; i++)
    {
      create_partition_name(name_buff, name, name_buffer_ptr, NORMAL_PART_NAME,
                            FALSE);
      if (!(m_file[i]= file[i]->clone(name_buff, m_clone_mem_root)))
      {
        error= HA_ERR_INITIALIZATION;
        file= &m_file[i];
        goto err_handler;
      }
      name_buffer_ptr+= strlen(name_buffer_ptr) + 1;
    }
  }
  else
  {
   file= m_file;
   do
   {
      create_partition_name(name_buff, name, name_buffer_ptr, NORMAL_PART_NAME,
                            FALSE);
      table->s->connect_string = m_connect_string[(uint)(file-m_file)];
      if ((error= (*file)->ha_open(table, name_buff, mode, test_if_locked)))
        goto err_handler;
      bzero(&table->s->connect_string, sizeof(LEX_STRING));
      m_num_locks+= (*file)->lock_count();
      name_buffer_ptr+= strlen(name_buffer_ptr) + 1;
    } while (*(++file));
  }
  
  file= m_file;
  ref_length= (*file)->ref_length;
  check_table_flags= (((*file)->ha_table_flags() &
                       ~(PARTITION_DISABLED_TABLE_FLAGS)) |
                      (PARTITION_ENABLED_TABLE_FLAGS));
  while (*(++file))
  {
    /* MyISAM can have smaller ref_length for partitions with MAX_ROWS set */
    set_if_bigger(ref_length, ((*file)->ref_length));
    /*
      Verify that all partitions have the same set of table flags.
      Mask all flags that partitioning enables/disables.
    */
    if (check_table_flags != (((*file)->ha_table_flags() &
                               ~(PARTITION_DISABLED_TABLE_FLAGS)) |
                              (PARTITION_ENABLED_TABLE_FLAGS)))
    {
      error= HA_ERR_INITIALIZATION;
      /* set file to last handler, so all of them is closed */
      file = &m_file[m_tot_parts - 1];
      goto err_handler;
    }
  }
  key_used_on_scan= m_file[0]->key_used_on_scan;
  implicit_emptied= m_file[0]->implicit_emptied;
  /*
    Add 2 bytes for partition id in position ref length.
    ref_length=max_in_all_partitions(ref_length) + PARTITION_BYTES_IN_POS
  */
  ref_length+= PARTITION_BYTES_IN_POS;
  m_ref_length= ref_length;

  /*
    Release buffer read from .par file. It will not be reused again after
    being opened once.
  */
  clear_handler_file();

  /*
    Use table_share->ha_part_data to share auto_increment_value among
    all handlers for the same table.
  */
  if (is_not_tmp_table)
    mysql_mutex_lock(&table_share->LOCK_ha_data);
  if (!table_share->ha_part_data)
  {
    /* currently only needed for auto_increment */
    table_share->ha_part_data= (HA_DATA_PARTITION*)
                                   alloc_root(&table_share->mem_root,
                                              sizeof(HA_DATA_PARTITION));
    if (!table_share->ha_part_data)
    {
      if (is_not_tmp_table)
        mysql_mutex_unlock(&table_share->LOCK_ha_data);
      goto err_handler;
    }
    DBUG_PRINT("info", ("table_share->ha_part_data 0x%p",
                        table_share->ha_part_data));
    bzero(table_share->ha_part_data, sizeof(HA_DATA_PARTITION));
    table_share->ha_part_data_destroy= ha_data_partition_destroy;
    mysql_mutex_init(key_PARTITION_LOCK_auto_inc,
                     &table_share->ha_part_data->LOCK_auto_inc,
                     MY_MUTEX_INIT_FAST);
  }
  if (is_not_tmp_table)
    mysql_mutex_unlock(&table_share->LOCK_ha_data);
  /*
    Some handlers update statistics as part of the open call. This will in
    some cases corrupt the statistics of the partition handler and thus
    to ensure we have correct statistics we call info from open after
    calling open on all individual handlers.
  */
  m_handler_status= handler_opened;
  if (m_part_info->part_expr)
    m_part_func_monotonicity_info=
                            m_part_info->part_expr->get_monotonicity_info();
  else if (m_part_info->list_of_part_fields)
    m_part_func_monotonicity_info= MONOTONIC_STRICT_INCREASING;
  info(HA_STATUS_VARIABLE | HA_STATUS_CONST);
  DBUG_RETURN(0);

err_handler:
  DEBUG_SYNC(ha_thd(), "partition_open_error");
  while (file-- != m_file)
    (*file)->ha_close();
err_alloc:
  bitmap_free(&m_bulk_insert_started);
  bitmap_free(&m_key_not_found_partitions);
  if (!m_is_clone_of)
    bitmap_free(&(m_part_info->used_partitions));

  DBUG_RETURN(error);
}