create_tmp_table(THD *thd, TMP_TABLE_PARAM *param, List<Item> &fields,
		 ORDER *group, bool distinct, bool save_sum_fields,
		 ulonglong select_options, ha_rows rows_limit,
                 const char *table_alias, bool do_not_open,
                 bool keep_row_order)
{
  MEM_ROOT *mem_root_save, own_root;
  TABLE *table;
  TABLE_SHARE *share;
  uint	i,field_count,null_count,null_pack_length;
  uint  copy_func_count= param->func_count;
  uint  hidden_null_count, hidden_null_pack_length, hidden_field_count;
  uint  blob_count,group_null_items, string_count;
  uint  temp_pool_slot=MY_BIT_NONE;
  uint fieldnr= 0;
  ulong reclength, string_total_length;
  bool  using_unique_constraint= false;
  bool  use_packed_rows= false;
  bool  not_all_columns= !(select_options & TMP_TABLE_ALL_COLUMNS);
  char  *tmpname,path[FN_REFLEN];
  uchar	*pos, *group_buff, *bitmaps;
  uchar *null_flags;
  Field **reg_field, **from_field, **default_field;
  uint *blob_field;
  Copy_field *copy=0;
  KEY *keyinfo;
  KEY_PART_INFO *key_part_info;
  Item **copy_func;
  TMP_ENGINE_COLUMNDEF *recinfo;
  /*
    total_uneven_bit_length is uneven bit length for visible fields
    hidden_uneven_bit_length is uneven bit length for hidden fields
  */
  uint total_uneven_bit_length= 0, hidden_uneven_bit_length= 0;
  bool force_copy_fields= param->force_copy_fields;
  /* Treat sum functions as normal ones when loose index scan is used. */
  save_sum_fields|= param->precomputed_group_by;
  DBUG_ENTER("create_tmp_table");
  DBUG_PRINT("enter",
             ("table_alias: '%s'  distinct: %d  save_sum_fields: %d  "
              "rows_limit: %lu  group: %d", table_alias,
              (int) distinct, (int) save_sum_fields,
              (ulong) rows_limit, MY_TEST(group)));

  thd->query_plan_flags|= QPLAN_TMP_TABLE;

  if (use_temp_pool && !(test_flags & TEST_KEEP_TMP_TABLES))
    temp_pool_slot = bitmap_lock_set_next(&temp_pool);

  if (temp_pool_slot != MY_BIT_NONE) // we got a slot
    sprintf(path, "%s_%lx_%i", tmp_file_prefix,
            current_pid, temp_pool_slot);
  else
  {
    /* if we run out of slots or we are not using tempool */
    sprintf(path, "%s%lx_%lx_%x", tmp_file_prefix,current_pid,
            (ulong) thd->thread_id, thd->tmp_table++);
  }

  /*
    No need to change table name to lower case as we are only creating
    MyISAM, Aria or HEAP tables here
  */
  fn_format(path, path, mysql_tmpdir, "",
            MY_REPLACE_EXT|MY_UNPACK_FILENAME);

  if (group)
  {
    ORDER **prev= &group;
    if (!param->quick_group)
      group=0;					// Can't use group key
    else for (ORDER *tmp=group ; tmp ; tmp=tmp->next)
    {
      /* Exclude found constant from the list */
      if ((*tmp->item)->const_item())
      {
        *prev= tmp->next;
        param->group_parts--;
        continue;
      }
      else
        prev= &(tmp->next);
      /*
        marker == 4 means two things:
        - store NULLs in the key, and
        - convert BIT fields to 64-bit long, needed because MEMORY tables
          can't index BIT fields.
      */
      (*tmp->item)->marker=4;			// Store null in key
      if ((*tmp->item)->too_big_for_varchar())
	using_unique_constraint= true;
    }
    if (param->group_length >= MAX_BLOB_WIDTH)
      using_unique_constraint= true;
    if (group)
      distinct=0;				// Can't use distinct
  }

  field_count=param->field_count+param->func_count+param->sum_func_count;
  hidden_field_count=param->hidden_field_count;

  /*
    When loose index scan is employed as access method, it already
    computes all groups and the result of all aggregate functions. We
    make space for the items of the aggregate function in the list of
    functions TMP_TABLE_PARAM::items_to_copy, so that the values of
    these items are stored in the temporary table.
  */
  if (param->precomputed_group_by)
    copy_func_count+= param->sum_func_count;
  
  init_sql_alloc(&own_root, TABLE_ALLOC_BLOCK_SIZE, 0, MYF(MY_THREAD_SPECIFIC));

  if (!multi_alloc_root(&own_root,
                        &table, sizeof(*table),
                        &share, sizeof(*share),
                        &reg_field, sizeof(Field*) * (field_count+1),
                        &default_field, sizeof(Field*) * (field_count),
                        &blob_field, sizeof(uint)*(field_count+1),
                        &from_field, sizeof(Field*)*field_count,
                        &copy_func, sizeof(*copy_func)*(copy_func_count+1),
                        &param->keyinfo, sizeof(*param->keyinfo),
                        &key_part_info,
                        sizeof(*key_part_info)*(param->group_parts+1),
                        &param->start_recinfo,
                        sizeof(*param->recinfo)*(field_count*2+4),
                        &tmpname, (uint) strlen(path)+1,
                        &group_buff, (group && ! using_unique_constraint ?
                                      param->group_length : 0),
                        &bitmaps, bitmap_buffer_size(field_count)*6,
                        NullS))
  {
    if (temp_pool_slot != MY_BIT_NONE)
      bitmap_lock_clear_bit(&temp_pool, temp_pool_slot);
    DBUG_RETURN(NULL);				/* purecov: inspected */
  }
  /* Copy_field belongs to TMP_TABLE_PARAM, allocate it in THD mem_root */
  if (!(param->copy_field= copy= new (thd->mem_root) Copy_field[field_count]))
  {
    if (temp_pool_slot != MY_BIT_NONE)
      bitmap_lock_clear_bit(&temp_pool, temp_pool_slot);
    free_root(&own_root, MYF(0));               /* purecov: inspected */
    DBUG_RETURN(NULL);				/* purecov: inspected */
  }
  param->items_to_copy= copy_func;
  strmov(tmpname, path);
  /* make table according to fields */

  bzero((char*) table,sizeof(*table));
  bzero((char*) reg_field,sizeof(Field*)*(field_count+1));
  bzero((char*) default_field, sizeof(Field*) * (field_count));
  bzero((char*) from_field,sizeof(Field*)*field_count);

  table->mem_root= own_root;
  mem_root_save= thd->mem_root;
  thd->mem_root= &table->mem_root;

  table->field=reg_field;
  table->alias.set(table_alias, strlen(table_alias), table_alias_charset);

  table->reginfo.lock_type=TL_WRITE;	/* Will be updated */
  table->map=1;
  table->temp_pool_slot = temp_pool_slot;
  table->copy_blobs= 1;
  table->in_use= thd;
  table->quick_keys.init();
  table->covering_keys.init();
  table->intersect_keys.init();
  table->keys_in_use_for_query.init();
  table->no_rows_with_nulls= param->force_not_null_cols;

  table->s= share;
  init_tmp_table_share(thd, share, "", 0, "(temporary)", tmpname);
  share->blob_field= blob_field;
  share->table_charset= param->table_charset;
  share->primary_key= MAX_KEY;               // Indicate no primary key
  share->keys_for_keyread.init();
  share->keys_in_use.init();
  if (param->schema_table)
    share->db= INFORMATION_SCHEMA_NAME;

  /* Calculate which type of fields we will store in the temporary table */

  reclength= string_total_length= 0;
  blob_count= string_count= null_count= hidden_null_count= group_null_items= 0;
  param->using_outer_summary_function= 0;

  List_iterator_fast<Item> li(fields);
  Item *item;
  Field **tmp_from_field=from_field;
  while ((item=li++))
  {
    Item::Type type= item->type();
    if (type == Item::COPY_STR_ITEM)
    {
      item= ((Item_copy *)item)->get_item();
      type= item->type();
    }
    if (not_all_columns)
    {
      if (item->with_sum_func && type != Item::SUM_FUNC_ITEM)
      {
        if (item->used_tables() & OUTER_REF_TABLE_BIT)
          item->update_used_tables();
        if ((item->real_type() == Item::SUBSELECT_ITEM) ||
            (item->used_tables() & ~OUTER_REF_TABLE_BIT))
        {
	  /*
	    Mark that the we have ignored an item that refers to a summary
	    function. We need to know this if someone is going to use
	    DISTINCT on the result.
	  */
	  param->using_outer_summary_function=1;
	  continue;
        }
      }
      if (item->const_item() && (int) hidden_field_count <= 0)
        continue; // We don't have to store this
    }
    if (type == Item::SUM_FUNC_ITEM && !group && !save_sum_fields)
    {						/* Can't calc group yet */
      Item_sum *sum_item= (Item_sum *) item;
      sum_item->result_field=0;
      for (i=0 ; i < sum_item->get_arg_count() ; i++)
      {
	Item *arg= sum_item->get_arg(i);
	if (!arg->const_item())
	{
	  Field *new_field=
            create_tmp_field(thd, table, arg, arg->type(), &copy_func,
                             tmp_from_field, &default_field[fieldnr],
                             group != 0,not_all_columns,
                             distinct, false);
	  if (!new_field)
	    goto err;					// Should be OOM
	  tmp_from_field++;
	  reclength+=new_field->pack_length();
	  if (new_field->flags & BLOB_FLAG)
	  {
	    *blob_field++= fieldnr;
	    blob_count++;
	  }
          if (new_field->type() == MYSQL_TYPE_BIT)
            total_uneven_bit_length+= new_field->field_length & 7;
	  *(reg_field++)= new_field;
          if (new_field->real_type() == MYSQL_TYPE_STRING ||
              new_field->real_type() == MYSQL_TYPE_VARCHAR)
          {
            string_count++;
            string_total_length+= new_field->pack_length();
          }
          thd->mem_root= mem_root_save;
          arg= sum_item->set_arg(i, thd, new (thd->mem_root) Item_temptable_field(thd, new_field));
          thd->mem_root= &table->mem_root;
          if (param->force_not_null_cols)
	  {
            new_field->flags|= NOT_NULL_FLAG;
            new_field->null_ptr= NULL;
          }
	  if (!(new_field->flags & NOT_NULL_FLAG))
          {
	    null_count++;
            /*
              new_field->maybe_null() is still false, it will be
              changed below. But we have to setup Item_field correctly
            */
            arg->maybe_null=1;
          }
          new_field->field_index= fieldnr++;
	}
      }
    }
    else
    {
      /*
	The last parameter to create_tmp_field() is a bit tricky:

	We need to set it to 0 in union, to get fill_record() to modify the
	temporary table.
	We need to set it to 1 on multi-table-update and in select to
	write rows to the temporary table.
	We here distinguish between UNION and multi-table-updates by the fact
	that in the later case group is set to the row pointer.

        The test for item->marker == 4 is ensure we don't create a group-by
        key over a bit field as heap tables can't handle that.
      */
      Field *new_field= (param->schema_table) ?
        item->create_field_for_schema(thd, table) :
        create_tmp_field(thd, table, item, type, &copy_func,
                         tmp_from_field, &default_field[fieldnr],
                         group != 0,
                         !force_copy_fields &&
                           (not_all_columns || group !=0),
                         /*
                           If item->marker == 4 then we force create_tmp_field
                           to create a 64-bit longs for BIT fields because HEAP
                           tables can't index BIT fields directly. We do the
                           same for distinct, as we want the distinct index
                           to be usable in this case too.
                         */
                         item->marker == 4  || param->bit_fields_as_long,
                         force_copy_fields);

      if (!new_field)
      {
	if (thd->is_fatal_error)
	  goto err;				// Got OOM
	continue;				// Some kind of const item
      }
      if (type == Item::SUM_FUNC_ITEM)
      {
        Item_sum *agg_item= (Item_sum *) item;
        /*
          Update the result field only if it has never been set, or if the
          created temporary table is not to be used for subquery
          materialization.

          The reason is that for subqueries that require
          materialization as part of their plan, we create the
          'external' temporary table needed for IN execution, after
          the 'internal' temporary table needed for grouping.  Since
          both the external and the internal temporary tables are
          created for the same list of SELECT fields of the subquery,
          setting 'result_field' for each invocation of
          create_tmp_table overrides the previous value of
          'result_field'.

          The condition below prevents the creation of the external
          temp table to override the 'result_field' that was set for
          the internal temp table.
        */
        if (!agg_item->result_field || !param->materialized_subquery)
          agg_item->result_field= new_field;
      }
      tmp_from_field++;
      if (param->force_not_null_cols)
      {
        new_field->flags|= NOT_NULL_FLAG;
        new_field->null_ptr= NULL;
      }
      reclength+=new_field->pack_length();
      if (!(new_field->flags & NOT_NULL_FLAG))
	null_count++;
      if (new_field->type() == MYSQL_TYPE_BIT)
        total_uneven_bit_length+= new_field->field_length & 7;
      if (new_field->flags & BLOB_FLAG)
      {
        *blob_field++= fieldnr;
	blob_count++;
      }

      if (new_field->real_type() == MYSQL_TYPE_STRING ||
          new_field->real_type() == MYSQL_TYPE_VARCHAR)
      {
        string_count++;
        string_total_length+= new_field->pack_length();
      }

      if (item->marker == 4 && item->maybe_null)
      {
	group_null_items++;
	new_field->flags|= GROUP_FLAG;
      }
      new_field->field_index= fieldnr++;
      *(reg_field++)= new_field;
    }
    if (!--hidden_field_count)
    {
      /*
        This was the last hidden field; Remember how many hidden fields could
        have null
      */
      hidden_null_count=null_count;
      /*
	We need to update hidden_field_count as we may have stored group
	functions with constant arguments
      */
      param->hidden_field_count= fieldnr;
      null_count= 0;
      /*
        On last hidden field we store uneven bit length in
        hidden_uneven_bit_length and proceed calculation of
        uneven bits for visible fields into
        total_uneven_bit_length variable.
      */
      hidden_uneven_bit_length= total_uneven_bit_length;
      total_uneven_bit_length= 0;
    }
  }
  DBUG_ASSERT(fieldnr == (uint) (reg_field - table->field));
  DBUG_ASSERT(field_count >= (uint) (reg_field - table->field));
  field_count= fieldnr;
  *reg_field= 0;
  *blob_field= 0;				// End marker
  share->fields= field_count;
  share->column_bitmap_size= bitmap_buffer_size(share->fields);

  /* If result table is small; use a heap */
  /* future: storage engine selection can be made dynamic? */
  if (blob_count || using_unique_constraint
      || (thd->variables.big_tables && !(select_options & SELECT_SMALL_RESULT))
      || (select_options & TMP_TABLE_FORCE_MYISAM)
      || thd->variables.tmp_memory_table_size == 0)
  {
    share->db_plugin= ha_lock_engine(0, TMP_ENGINE_HTON);
    table->file= get_new_handler(share, &table->mem_root,
                                 share->db_type());
    if (group &&
	(param->group_parts > table->file->max_key_parts() ||
	 param->group_length > table->file->max_key_length()))
      using_unique_constraint= true;
  }
  else
  {
    share->db_plugin= ha_lock_engine(0, heap_hton);
    table->file= get_new_handler(share, &table->mem_root,
                                 share->db_type());
  }
  if (!table->file)
    goto err;

  if (table->file->set_ha_share_ref(&share->ha_share))
  {
    delete table->file;
    goto err;
  }

  if (!using_unique_constraint)
    reclength+= group_null_items;	// null flag is stored separately

  share->blob_fields= blob_count;
  if (blob_count == 0)
  {
    /* We need to ensure that first byte is not 0 for the delete link */
    if (param->hidden_field_count)
      hidden_null_count++;
    else
      null_count++;
  }
  hidden_null_pack_length= (hidden_null_count + 7 +
                            hidden_uneven_bit_length) / 8;
  null_pack_length= (hidden_null_pack_length +
                     (null_count + total_uneven_bit_length + 7) / 8);
  reclength+=null_pack_length;
  if (!reclength)
    reclength=1;				// Dummy select
  /* Use packed rows if there is blobs or a lot of space to gain */
  if (blob_count ||
      (string_total_length >= STRING_TOTAL_LENGTH_TO_PACK_ROWS &&
       (reclength / string_total_length <= RATIO_TO_PACK_ROWS ||
        string_total_length / string_count >= AVG_STRING_LENGTH_TO_PACK_ROWS)))
    use_packed_rows= 1;

  share->reclength= reclength;
  {
    uint alloc_length=ALIGN_SIZE(reclength+MI_UNIQUE_HASH_LENGTH+1);
    share->rec_buff_length= alloc_length;
    if (!(table->record[0]= (uchar*)
                            alloc_root(&table->mem_root, alloc_length*3)))
      goto err;
    table->record[1]= table->record[0]+alloc_length;
    share->default_values= table->record[1]+alloc_length;
  }
  copy_func[0]=0;				// End marker
  param->func_count= (uint)(copy_func - param->items_to_copy); 

  setup_tmp_table_column_bitmaps(table, bitmaps);

  recinfo=param->start_recinfo;
  null_flags=(uchar*) table->record[0];
  pos=table->record[0]+ null_pack_length;
  if (null_pack_length)
  {
    bzero((uchar*) recinfo,sizeof(*recinfo));
    recinfo->type=FIELD_NORMAL;
    recinfo->length=null_pack_length;
    recinfo++;
    bfill(null_flags,null_pack_length,255);	// Set null fields

    table->null_flags= (uchar*) table->record[0];
    share->null_fields= null_count+ hidden_null_count;
    share->null_bytes= share->null_bytes_for_compare= null_pack_length;
  }
  null_count= (blob_count == 0) ? 1 : 0;
  hidden_field_count=param->hidden_field_count;
  for (i=0,reg_field=table->field; i < field_count; i++,reg_field++,recinfo++)
  {
    Field *field= *reg_field;
    uint length;
    bzero((uchar*) recinfo,sizeof(*recinfo));

    if (!(field->flags & NOT_NULL_FLAG))
    {
      recinfo->null_bit= (uint8)1 << (null_count & 7);
      recinfo->null_pos= null_count/8;
      field->move_field(pos,null_flags+null_count/8,
			(uint8)1 << (null_count & 7));
      null_count++;
    }
    else
      field->move_field(pos,(uchar*) 0,0);
    if (field->type() == MYSQL_TYPE_BIT)
    {
      /* We have to reserve place for extra bits among null bits */
      ((Field_bit*) field)->set_bit_ptr(null_flags + null_count / 8,
                                        null_count & 7);
      null_count+= (field->field_length & 7);
    }
    field->reset();

    /*
      Test if there is a default field value. The test for ->ptr is to skip
      'offset' fields generated by initalize_tables
    */
    if (default_field[i] && default_field[i]->ptr)
    {
      /* 
         default_field[i] is set only in the cases  when 'field' can
         inherit the default value that is defined for the field referred
         by the Item_field object from which 'field' has been created.
      */
      const Field *orig_field= default_field[i];
      /* Get the value from default_values */
      if (orig_field->is_null_in_record(orig_field->table->s->default_values))
        field->set_null();
      else
      {
        field->set_notnull();
        memcpy(field->ptr,
               orig_field->ptr_in_record(orig_field->table->s->default_values),
               field->pack_length_in_rec());
      }
    } 

    if (from_field[i])
    {						/* Not a table Item */
      copy->set(field,from_field[i],save_sum_fields);
      copy++;
    }
    length=field->pack_length();
    pos+= length;

    /* Make entry for create table */
    recinfo->length=length;
    if (field->flags & BLOB_FLAG)
      recinfo->type= FIELD_BLOB;
    else if (use_packed_rows &&
             field->real_type() == MYSQL_TYPE_STRING &&
	     length >= MIN_STRING_LENGTH_TO_PACK_ROWS)
      recinfo->type= FIELD_SKIP_ENDSPACE;
    else if (field->real_type() == MYSQL_TYPE_VARCHAR)
      recinfo->type= FIELD_VARCHAR;
    else
      recinfo->type= FIELD_NORMAL;

    if (!--hidden_field_count)
      null_count=(null_count+7) & ~7;		// move to next byte

    // fix table name in field entry
    field->set_table_name(&table->alias);
  }

  param->copy_field_end=copy;
  param->recinfo= recinfo;              	// Pointer to after last field
  store_record(table,s->default_values);        // Make empty default record

  if (thd->variables.tmp_memory_table_size == ~ (ulonglong) 0)	// No limit
    share->max_rows= ~(ha_rows) 0;
  else
    share->max_rows= (ha_rows) (((share->db_type() == heap_hton) ?
                                 MY_MIN(thd->variables.tmp_memory_table_size,
                                     thd->variables.max_heap_table_size) :
                                 thd->variables.tmp_memory_table_size) /
                                share->reclength);
  set_if_bigger(share->max_rows,1);		// For dummy start options
  /*
    Push the LIMIT clause to the temporary table creation, so that we
    materialize only up to 'rows_limit' records instead of all result records.
  */
  set_if_smaller(share->max_rows, rows_limit);
  param->end_write_records= rows_limit;

  keyinfo= param->keyinfo;

  if (group)
  {
    DBUG_PRINT("info",("Creating group key in temporary table"));
    table->group=group;				/* Table is grouped by key */
    param->group_buff=group_buff;
    share->keys=1;
    share->uniques= MY_TEST(using_unique_constraint);
    table->key_info= table->s->key_info= keyinfo;
    table->keys_in_use_for_query.set_bit(0);
    share->keys_in_use.set_bit(0);
    keyinfo->key_part=key_part_info;
    keyinfo->flags=HA_NOSAME | HA_BINARY_PACK_KEY | HA_PACK_KEY;
    keyinfo->ext_key_flags= keyinfo->flags;
    keyinfo->usable_key_parts=keyinfo->user_defined_key_parts= param->group_parts;
    keyinfo->ext_key_parts= keyinfo->user_defined_key_parts;
    keyinfo->key_length=0;
    keyinfo->rec_per_key=NULL;
    keyinfo->read_stats= NULL;
    keyinfo->collected_stats= NULL;
    keyinfo->algorithm= HA_KEY_ALG_UNDEF;
    keyinfo->is_statistics_from_stat_tables= FALSE;
    keyinfo->name= (char*) "group_key";
    ORDER *cur_group= group;
    for (; cur_group ; cur_group= cur_group->next, key_part_info++)
    {
      Field *field=(*cur_group->item)->get_tmp_table_field();
      DBUG_ASSERT(field->table == table);
      bool maybe_null=(*cur_group->item)->maybe_null;
      key_part_info->null_bit=0;
      key_part_info->field=  field;
      key_part_info->fieldnr= field->field_index + 1;
      if (cur_group == group)
        field->key_start.set_bit(0);
      key_part_info->offset= field->offset(table->record[0]);
      key_part_info->length= (uint16) field->key_length();
      key_part_info->type=   (uint8) field->key_type();
      key_part_info->key_type =
	((ha_base_keytype) key_part_info->type == HA_KEYTYPE_TEXT ||
	 (ha_base_keytype) key_part_info->type == HA_KEYTYPE_VARTEXT1 ||
	 (ha_base_keytype) key_part_info->type == HA_KEYTYPE_VARTEXT2) ?
	0 : FIELDFLAG_BINARY;
      key_part_info->key_part_flag= 0;
      if (!using_unique_constraint)
      {
	cur_group->buff=(char*) group_buff;

        if (maybe_null && !field->null_bit)
        {
          /*
            This can only happen in the unusual case where an outer join
            table was found to be not-nullable by the optimizer and we
            the item can't really be null.
            We solve this by marking the item as !maybe_null to ensure
            that the key,field and item definition match.
          */
          (*cur_group->item)->maybe_null= maybe_null= 0;
        }

	if (!(cur_group->field= field->new_key_field(thd->mem_root,table,
                                                     group_buff +
                                                     MY_TEST(maybe_null),
                                                     key_part_info->length,
                                                     field->null_ptr,
                                                     field->null_bit)))
	  goto err; /* purecov: inspected */

	if (maybe_null)
	{
	  /*
	    To be able to group on NULL, we reserved place in group_buff
	    for the NULL flag just before the column. (see above).
	    The field data is after this flag.
	    The NULL flag is updated in 'end_update()' and 'end_write()'
	  */
	  keyinfo->flags|= HA_NULL_ARE_EQUAL;	// def. that NULL == NULL
	  key_part_info->null_bit=field->null_bit;
	  key_part_info->null_offset= (uint) (field->null_ptr -
					      (uchar*) table->record[0]);
          cur_group->buff++;                        // Pointer to field data
	  group_buff++;                         // Skipp null flag
	}
	group_buff+= cur_group->field->pack_length();
      }
      keyinfo->key_length+=  key_part_info->length;
    }
    /*
      Ensure we didn't overrun the group buffer. The < is only true when
      some maybe_null fields was changed to be not null fields.
    */
    DBUG_ASSERT(using_unique_constraint ||
                group_buff <= param->group_buff + param->group_length);
  }

  if (distinct && field_count != param->hidden_field_count)
  {
    /*
      Create an unique key or an unique constraint over all columns
      that should be in the result.  In the temporary table, there are
      'param->hidden_field_count' extra columns, whose null bits are stored
      in the first 'hidden_null_pack_length' bytes of the row.
    */
    DBUG_PRINT("info",("hidden_field_count: %d", param->hidden_field_count));

    if (blob_count)
    {
      /*
        Special mode for index creation in MyISAM used to support unique
        indexes on blobs with arbitrary length. Such indexes cannot be
        used for lookups.
      */
      share->uniques= 1;
    }
    null_pack_length-=hidden_null_pack_length;
    keyinfo->user_defined_key_parts=
      ((field_count-param->hidden_field_count)+
       (share->uniques ? MY_TEST(null_pack_length) : 0));
    keyinfo->ext_key_parts= keyinfo->user_defined_key_parts;
    keyinfo->usable_key_parts= keyinfo->user_defined_key_parts;
    table->distinct= 1;
    share->keys= 1;
    if (!(key_part_info= (KEY_PART_INFO*)
          alloc_root(&table->mem_root,
                     keyinfo->user_defined_key_parts * sizeof(KEY_PART_INFO))))
      goto err;
    bzero((void*) key_part_info, keyinfo->user_defined_key_parts * sizeof(KEY_PART_INFO));
    table->keys_in_use_for_query.set_bit(0);
    share->keys_in_use.set_bit(0);
    table->key_info= table->s->key_info= keyinfo;
    keyinfo->key_part=key_part_info;
    keyinfo->flags=HA_NOSAME | HA_NULL_ARE_EQUAL | HA_BINARY_PACK_KEY | HA_PACK_KEY;
    keyinfo->ext_key_flags= keyinfo->flags;
    keyinfo->key_length= 0;  // Will compute the sum of the parts below.
    keyinfo->name= (char*) "distinct_key";
    keyinfo->algorithm= HA_KEY_ALG_UNDEF;
    keyinfo->is_statistics_from_stat_tables= FALSE;
    keyinfo->read_stats= NULL;
    keyinfo->collected_stats= NULL;

    /*
      Needed by non-merged semi-joins: SJ-Materialized table must have a valid 
      rec_per_key array, because it participates in join optimization. Since
      the table has no data, the only statistics we can provide is "unknown",
      i.e. zero values.

      (For table record count, we calculate and set JOIN_TAB::found_records,
       see get_delayed_table_estimates()).
    */
    size_t rpk_size= keyinfo->user_defined_key_parts * sizeof(keyinfo->rec_per_key[0]);
    if (!(keyinfo->rec_per_key= (ulong*) alloc_root(&table->mem_root, 
                                                    rpk_size)))
      goto err;
    bzero(keyinfo->rec_per_key, rpk_size);

    /*
      Create an extra field to hold NULL bits so that unique indexes on
      blobs can distinguish NULL from 0. This extra field is not needed
      when we do not use UNIQUE indexes for blobs.
    */
    if (null_pack_length && share->uniques)
    {
      key_part_info->null_bit=0;
      key_part_info->offset=hidden_null_pack_length;
      key_part_info->length=null_pack_length;
      key_part_info->field= new Field_string(table->record[0],
                                             (uint32) key_part_info->length,
                                             (uchar*) 0,
                                             (uint) 0,
                                             Field::NONE,
                                             NullS, &my_charset_bin);
      if (!key_part_info->field)
        goto err;
      key_part_info->field->init(table);
      key_part_info->key_type=FIELDFLAG_BINARY;
      key_part_info->type=    HA_KEYTYPE_BINARY;
      key_part_info->fieldnr= key_part_info->field->field_index + 1;
      key_part_info++;
    }
    /* Create a distinct key over the columns we are going to return */
    for (i=param->hidden_field_count, reg_field=table->field + i ;
	 i < field_count;
	 i++, reg_field++, key_part_info++)
    {
      key_part_info->field=    *reg_field;
      (*reg_field)->flags |= PART_KEY_FLAG;
      if (key_part_info == keyinfo->key_part)
        (*reg_field)->key_start.set_bit(0);
      key_part_info->null_bit= (*reg_field)->null_bit;
      key_part_info->null_offset= (uint) ((*reg_field)->null_ptr -
                                          (uchar*) table->record[0]);

      key_part_info->offset=   (*reg_field)->offset(table->record[0]);
      key_part_info->length=   (uint16) (*reg_field)->pack_length();
      key_part_info->fieldnr= (*reg_field)->field_index + 1;
      /* TODO:
        The below method of computing the key format length of the
        key part is a copy/paste from opt_range.cc, and table.cc.
        This should be factored out, e.g. as a method of Field.
        In addition it is not clear if any of the Field::*_length
        methods is supposed to compute the same length. If so, it
        might be reused.
      */
      key_part_info->store_length= key_part_info->length;

      if ((*reg_field)->real_maybe_null())
      {
        key_part_info->store_length+= HA_KEY_NULL_LENGTH;
        key_part_info->key_part_flag |= HA_NULL_PART;
      }
      if ((*reg_field)->type() == MYSQL_TYPE_BLOB ||
          (*reg_field)->real_type() == MYSQL_TYPE_VARCHAR ||
          (*reg_field)->type() == MYSQL_TYPE_GEOMETRY)
      {
        if ((*reg_field)->type() == MYSQL_TYPE_BLOB ||
            (*reg_field)->type() == MYSQL_TYPE_GEOMETRY)
          key_part_info->key_part_flag|= HA_BLOB_PART;
        else
          key_part_info->key_part_flag|= HA_VAR_LENGTH_PART;

        key_part_info->store_length+=HA_KEY_BLOB_LENGTH;
      }

      keyinfo->key_length+= key_part_info->store_length;

      key_part_info->type=     (uint8) (*reg_field)->key_type();
      key_part_info->key_type =
	((ha_base_keytype) key_part_info->type == HA_KEYTYPE_TEXT ||
	 (ha_base_keytype) key_part_info->type == HA_KEYTYPE_VARTEXT1 ||
	 (ha_base_keytype) key_part_info->type == HA_KEYTYPE_VARTEXT2) ?
	0 : FIELDFLAG_BINARY;
    }
  }

  if (thd->is_fatal_error)				// If end of memory
    goto err;					 /* purecov: inspected */
  share->db_record_offset= 1;
  table->used_for_duplicate_elimination= (param->sum_func_count == 0 &&
                                          (table->group || table->distinct));
  table->keep_row_order= keep_row_order;

  if (!do_not_open)
  {
    if (instantiate_tmp_table(table, param->keyinfo, param->start_recinfo,
                              &param->recinfo, select_options))
      goto err;
  }

  // Make empty record so random data is not written to disk
  empty_record(table);
  table->status= STATUS_NO_RECORD;
  thd->mem_root= mem_root_save;

  DBUG_RETURN(table);

err:
  thd->mem_root= mem_root_save;
  free_tmp_table(thd,table);                    /* purecov: inspected */
  if (temp_pool_slot != MY_BIT_NONE)
    bitmap_lock_clear_bit(&temp_pool, temp_pool_slot);
  DBUG_RETURN(NULL);				/* purecov: inspected */
}