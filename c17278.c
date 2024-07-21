enum open_frm_error open_table_from_share(THD *thd, TABLE_SHARE *share,
                       const LEX_CSTRING *alias, uint db_stat, uint prgflag,
                       uint ha_open_flags, TABLE *outparam,
                       bool is_create_table, List<String> *partitions_to_open)
{
  enum open_frm_error error;
  uint records, i, bitmap_size, bitmap_count;
  const char *tmp_alias;
  bool error_reported= FALSE;
  uchar *record, *bitmaps;
  Field **field_ptr;
  uint8 save_context_analysis_only= thd->lex->context_analysis_only;
  TABLE_SHARE::enum_v_keys check_set_initialized= share->check_set_initialized;
  DBUG_ENTER("open_table_from_share");
  DBUG_PRINT("enter",("name: '%s.%s'  form: %p", share->db.str,
                      share->table_name.str, outparam));

  thd->lex->context_analysis_only&= ~CONTEXT_ANALYSIS_ONLY_VIEW; // not a view

  error= OPEN_FRM_ERROR_ALREADY_ISSUED; // for OOM errors below
  bzero((char*) outparam, sizeof(*outparam));
  outparam->in_use= thd;
  outparam->s= share;
  outparam->db_stat= db_stat;
  outparam->write_row_record= NULL;

  if (share->incompatible_version &&
      !(ha_open_flags & (HA_OPEN_FOR_ALTER | HA_OPEN_FOR_REPAIR)))
  {
    /* one needs to run mysql_upgrade on the table */
    error= OPEN_FRM_NEEDS_REBUILD;
    goto err;
  }
  init_sql_alloc(&outparam->mem_root, "table", TABLE_ALLOC_BLOCK_SIZE, 0,
                 MYF(0));

  /*
    We have to store the original alias in mem_root as constraints and virtual
    functions may store pointers to it
  */
  if (!(tmp_alias= strmake_root(&outparam->mem_root, alias->str, alias->length)))
    goto err;

  outparam->alias.set(tmp_alias, alias->length, table_alias_charset);
  outparam->quick_keys.init();
  outparam->covering_keys.init();
  outparam->intersect_keys.init();
  outparam->keys_in_use_for_query.init();

  /* Allocate handler */
  outparam->file= 0;
  if (!(prgflag & OPEN_FRM_FILE_ONLY))
  {
    if (!(outparam->file= get_new_handler(share, &outparam->mem_root,
                                          share->db_type())))
      goto err;

    if (outparam->file->set_ha_share_ref(&share->ha_share))
      goto err;
  }
  else
  {
    DBUG_ASSERT(!db_stat);
  }

  if (share->sequence && outparam->file)
  {
    ha_sequence *file;
    /* SEQUENCE table. Create a sequence handler over the original handler */
    if (!(file= (ha_sequence*) sql_sequence_hton->create(sql_sequence_hton, share,
                                                     &outparam->mem_root)))
      goto err;
    file->register_original_handler(outparam->file);
    outparam->file= file;
  }

  outparam->reginfo.lock_type= TL_UNLOCK;
  outparam->current_lock= F_UNLCK;
  records=0;
  if ((db_stat & HA_OPEN_KEYFILE) || (prgflag & DELAYED_OPEN))
    records=1;
  if (prgflag & (READ_ALL + EXTRA_RECORD))
  {
    records++;
    if (share->versioned)
      records++;
  }

  if (records == 0)
  {
    /* We are probably in hard repair, and the buffers should not be used */
    record= share->default_values;
  }
  else
  {
    if (!(record= (uchar*) alloc_root(&outparam->mem_root,
                                      share->rec_buff_length * records)))
      goto err;                                   /* purecov: inspected */
  }

  for (i= 0; i < 3;)
  {
    outparam->record[i]= record;
    if (++i < records)
      record+= share->rec_buff_length;
  }
  /* Mark bytes between records as not accessable to catch overrun bugs */
  for (i= 0; i < records; i++)
    MEM_NOACCESS(outparam->record[i] + share->reclength,
                 share->rec_buff_length - share->reclength);

  if (!(field_ptr = (Field **) alloc_root(&outparam->mem_root,
                                          (uint) ((share->fields+1)*
                                                  sizeof(Field*)))))
    goto err;                                   /* purecov: inspected */

  outparam->field= field_ptr;

  record= (uchar*) outparam->record[0]-1;	/* Fieldstart = 1 */
  if (share->null_field_first)
    outparam->null_flags= (uchar*) record+1;
  else
    outparam->null_flags= (uchar*) (record+ 1+ share->reclength -
                                    share->null_bytes);

  /* Setup copy of fields from share, but use the right alias and record */
  for (i=0 ; i < share->fields; i++, field_ptr++)
  {
    if (!((*field_ptr)= share->field[i]->clone(&outparam->mem_root, outparam)))
      goto err;
  }
  (*field_ptr)= 0;                              // End marker

  DEBUG_SYNC(thd, "TABLE_after_field_clone");

  outparam->vers_write= share->versioned;

  if (share->found_next_number_field)
    outparam->found_next_number_field=
      outparam->field[(uint) (share->found_next_number_field - share->field)];

  /* Fix key->name and key_part->field */
  if (share->key_parts)
  {
    KEY	*key_info, *key_info_end;
    KEY_PART_INFO *key_part;
    uint n_length;
    n_length= share->keys*sizeof(KEY) + share->ext_key_parts*sizeof(KEY_PART_INFO);
    if (!(key_info= (KEY*) alloc_root(&outparam->mem_root, n_length)))
      goto err;
    outparam->key_info= key_info;
    key_part= (reinterpret_cast<KEY_PART_INFO*>(key_info+share->keys));

    memcpy(key_info, share->key_info, sizeof(*key_info)*share->keys);
    memcpy(key_part, share->key_info[0].key_part, (sizeof(*key_part) *
                                                   share->ext_key_parts));

    for (key_info_end= key_info + share->keys ;
         key_info < key_info_end ;
         key_info++)
    {
      KEY_PART_INFO *key_part_end;

      key_info->table= outparam;
      key_info->key_part= key_part;

      key_part_end= key_part + (share->use_ext_keys ? key_info->ext_key_parts :
			                              key_info->user_defined_key_parts) ;
      for ( ; key_part < key_part_end; key_part++)
      {
        Field *field= key_part->field= outparam->field[key_part->fieldnr - 1];

        if (field->key_length() != key_part->length &&
            !(field->flags & BLOB_FLAG))
        {
          /*
            We are using only a prefix of the column as a key:
            Create a new field for the key part that matches the index
          */
          field= key_part->field=field->make_new_field(&outparam->mem_root,
                                                       outparam, 0);
          field->field_length= key_part->length;
        }
      }
      if (!share->use_ext_keys)
	key_part+= key_info->ext_key_parts - key_info->user_defined_key_parts;
    }
  }

  /*
    Process virtual and default columns, if any.
  */
  if (share->virtual_fields || share->default_fields ||
      share->default_expressions || share->table_check_constraints)
  {
    Field **vfield_ptr, **dfield_ptr;
    Virtual_column_info **check_constraint_ptr;

    if (!multi_alloc_root(&outparam->mem_root,
                          &vfield_ptr, (uint) ((share->virtual_fields + 1)*
                                               sizeof(Field*)),
                          &dfield_ptr, (uint) ((share->default_fields +
                                                share->default_expressions +1)*
                                               sizeof(Field*)),
                          &check_constraint_ptr,
                          (uint) ((share->table_check_constraints +
                                   share->field_check_constraints + 1)*
                                  sizeof(Virtual_column_info*)),
                          NullS))
      goto err;
    if (share->virtual_fields)
      outparam->vfield= vfield_ptr;
    if (share->default_fields + share->default_expressions)
      outparam->default_field= dfield_ptr;
    if (share->table_check_constraints || share->field_check_constraints)
      outparam->check_constraints= check_constraint_ptr;

    vcol_init_mode mode= VCOL_INIT_DEPENDENCY_FAILURE_IS_WARNING;
#if MYSQL_VERSION_ID > 100500
    switch (thd->lex->sql_command)
    {
    case SQLCOM_CREATE_TABLE:
      mode= VCOL_INIT_DEPENDENCY_FAILURE_IS_ERROR;
      break;
    case SQLCOM_ALTER_TABLE:
    case SQLCOM_CREATE_INDEX:
    case SQLCOM_DROP_INDEX:
      if ((ha_open_flags & HA_OPEN_FOR_ALTER) == 0)
        mode= VCOL_INIT_DEPENDENCY_FAILURE_IS_ERROR;
      break;
    default:
      break;
    }
#endif

    if (parse_vcol_defs(thd, &outparam->mem_root, outparam,
                        &error_reported, mode))
    {
      error= OPEN_FRM_CORRUPTED;
      goto err;
    }

    /* Update to use trigger fields */
    switch_defaults_to_nullable_trigger_fields(outparam);

    for (uint k= 0; k < share->keys; k++)
    {
      KEY &key_info= outparam->key_info[k];
      uint parts = (share->use_ext_keys ? key_info.ext_key_parts :
                    key_info.user_defined_key_parts);
      for (uint p= 0; p < parts; p++)
      {
        KEY_PART_INFO &kp= key_info.key_part[p];
        if (kp.field != outparam->field[kp.fieldnr - 1])
        {
          kp.field->vcol_info = outparam->field[kp.fieldnr - 1]->vcol_info;
        }
      }
    }
  }

#ifdef WITH_PARTITION_STORAGE_ENGINE
  bool work_part_info_used;
  if (share->partition_info_str_len && outparam->file)
  {
  /*
    In this execution we must avoid calling thd->change_item_tree since
    we might release memory before statement is completed. We do this
    by changing to a new statement arena. As part of this arena we also
    set the memory root to be the memory root of the table since we
    call the parser and fix_fields which both can allocate memory for
    item objects. We keep the arena to ensure that we can release the
    free_list when closing the table object.
    SEE Bug #21658
  */

    Query_arena *backup_stmt_arena_ptr= thd->stmt_arena;
    Query_arena backup_arena;
    Query_arena part_func_arena(&outparam->mem_root,
                                Query_arena::STMT_INITIALIZED);
    thd->set_n_backup_active_arena(&part_func_arena, &backup_arena);
    thd->stmt_arena= &part_func_arena;
    bool tmp;

    tmp= mysql_unpack_partition(thd, share->partition_info_str,
                                share->partition_info_str_len,
                                outparam, is_create_table,
                                plugin_hton(share->default_part_plugin),
                                &work_part_info_used);
    if (tmp)
    {
      thd->stmt_arena= backup_stmt_arena_ptr;
      thd->restore_active_arena(&part_func_arena, &backup_arena);
      goto partititon_err;
    }
    outparam->part_info->is_auto_partitioned= share->auto_partitioned;
    DBUG_PRINT("info", ("autopartitioned: %u", share->auto_partitioned));
    /* 
      We should perform the fix_partition_func in either local or
      caller's arena depending on work_part_info_used value.
    */
    if (!work_part_info_used)
      tmp= fix_partition_func(thd, outparam, is_create_table);
    thd->stmt_arena= backup_stmt_arena_ptr;
    thd->restore_active_arena(&part_func_arena, &backup_arena);
    if (!tmp)
    {
      if (work_part_info_used)
        tmp= fix_partition_func(thd, outparam, is_create_table);
    }
    outparam->part_info->item_free_list= part_func_arena.free_list;
partititon_err:
    if (tmp)
    {
      if (is_create_table)
      {
        /*
          During CREATE/ALTER TABLE it is ok to receive errors here.
          It is not ok if it happens during the opening of an frm
          file as part of a normal query.
        */
        error_reported= TRUE;
      }
      goto err;
    }
  }
#endif

  /* Check virtual columns against table's storage engine. */
  if (share->virtual_fields &&
        (outparam->file && 
          !(outparam->file->ha_table_flags() & HA_CAN_VIRTUAL_COLUMNS)))
  {
    my_error(ER_UNSUPPORTED_ENGINE_FOR_VIRTUAL_COLUMNS, MYF(0),
             plugin_name(share->db_plugin)->str);
    error_reported= TRUE;
    goto err;
  }

  /* Allocate bitmaps */

  bitmap_size= share->column_bitmap_size;
  bitmap_count= 7;
  if (share->virtual_fields)
    bitmap_count++;

  if (!(bitmaps= (uchar*) alloc_root(&outparam->mem_root,
                                     bitmap_size * bitmap_count)))
    goto err;

  my_bitmap_init(&outparam->def_read_set,
                 (my_bitmap_map*) bitmaps, share->fields, FALSE);
  bitmaps+= bitmap_size;
  my_bitmap_init(&outparam->def_write_set,
                 (my_bitmap_map*) bitmaps, share->fields, FALSE);
  bitmaps+= bitmap_size;

  /* Don't allocate vcol_bitmap if we don't need it */
  if (share->virtual_fields)
  {
    if (!(outparam->def_vcol_set= (MY_BITMAP*)
          alloc_root(&outparam->mem_root, sizeof(*outparam->def_vcol_set))))
      goto err;
    my_bitmap_init(outparam->def_vcol_set,
                   (my_bitmap_map*) bitmaps, share->fields, FALSE);
    bitmaps+= bitmap_size;
  }

  my_bitmap_init(&outparam->has_value_set,
                 (my_bitmap_map*) bitmaps, share->fields, FALSE);
  bitmaps+= bitmap_size;
  my_bitmap_init(&outparam->tmp_set,
                 (my_bitmap_map*) bitmaps, share->fields, FALSE);
  bitmaps+= bitmap_size;
  my_bitmap_init(&outparam->eq_join_set,
                 (my_bitmap_map*) bitmaps, share->fields, FALSE);
  bitmaps+= bitmap_size;
  my_bitmap_init(&outparam->cond_set,
                 (my_bitmap_map*) bitmaps, share->fields, FALSE);
  bitmaps+= bitmap_size;
  my_bitmap_init(&outparam->def_rpl_write_set,
                 (my_bitmap_map*) bitmaps, share->fields, FALSE);
  outparam->default_column_bitmaps();

  outparam->cond_selectivity= 1.0;

  /* The table struct is now initialized;  Open the table */
  if (db_stat)
  {
    if (specialflag & SPECIAL_WAIT_IF_LOCKED)
      ha_open_flags|= HA_OPEN_WAIT_IF_LOCKED;
    else
      ha_open_flags|= HA_OPEN_IGNORE_IF_LOCKED;

    int ha_err= outparam->file->ha_open(outparam, share->normalized_path.str,
                                 (db_stat & HA_READ_ONLY ? O_RDONLY : O_RDWR),
                                 ha_open_flags, 0, partitions_to_open);
    if (ha_err)
    {
      share->open_errno= ha_err;
      /* Set a flag if the table is crashed and it can be auto. repaired */
      share->crashed= (outparam->file->auto_repair(ha_err) &&
                       !(ha_open_flags & HA_OPEN_FOR_REPAIR));
      if (!thd->is_error())
        outparam->file->print_error(ha_err, MYF(0));
      error_reported= TRUE;

      if (ha_err == HA_ERR_TABLE_DEF_CHANGED)
        error= OPEN_FRM_DISCOVER;

      /*
        We're here, because .frm file was successfully opened.

        But if the table doesn't exist in the engine and the engine
        supports discovery, we force rediscover to discover
        the fact that table doesn't in fact exist and remove
        the stray .frm file.
      */
      if (share->db_type()->discover_table &&
          (ha_err == ENOENT || ha_err == HA_ERR_NO_SUCH_TABLE))
        error= OPEN_FRM_DISCOVER;

      goto err;
    }
  }

  outparam->mark_columns_used_by_virtual_fields();
  if (!check_set_initialized &&
      share->check_set_initialized == TABLE_SHARE::V_KEYS)
  {
    // copy PART_INDIRECT_KEY_FLAG that was set meanwhile by *some* thread
    for (uint i= 0 ; i < share->fields ; i++)
    {
      if (share->field[i]->flags & PART_INDIRECT_KEY_FLAG)
        outparam->field[i]->flags|= PART_INDIRECT_KEY_FLAG;
    }
  }

  if (db_stat)
  {
    /* Set some flags in share on first open of the table */
    handler::Table_flags flags= outparam->file->ha_table_flags();
    if (! MY_TEST(flags & (HA_BINLOG_STMT_CAPABLE |
                           HA_BINLOG_ROW_CAPABLE)) ||
        MY_TEST(flags & HA_HAS_OWN_BINLOGGING))
      share->no_replicate= TRUE;
    if (outparam->file->table_cache_type() & HA_CACHE_TBL_NOCACHE)
      share->not_usable_by_query_cache= TRUE;
  }

  if (share->no_replicate || !binlog_filter->db_ok(share->db.str))
    share->can_do_row_logging= 0;   // No row based replication

  /* Increment the opened_tables counter, only when open flags set. */
  if (db_stat)
    thd->status_var.opened_tables++;

  thd->lex->context_analysis_only= save_context_analysis_only;
  DBUG_RETURN (OPEN_FRM_OK);

 err:
  if (! error_reported)
    open_table_error(share, error, my_errno);
  delete outparam->file;
#ifdef WITH_PARTITION_STORAGE_ENGINE
  if (outparam->part_info)
    free_items(outparam->part_info->item_free_list);
#endif
  outparam->file= 0;				// For easier error checking
  outparam->db_stat=0;
  thd->lex->context_analysis_only= save_context_analysis_only;
  if (outparam->expr_arena)
    outparam->expr_arena->free_items();
  free_root(&outparam->mem_root, MYF(0));       // Safe to call on bzero'd root
  outparam->alias.free();
  DBUG_RETURN (error);
}