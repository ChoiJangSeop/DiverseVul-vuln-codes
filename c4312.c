int mi_repair_by_sort(MI_CHECK *param, register MI_INFO *info,
		      const char * name, int rep_quick)
{
  int got_error;
  uint i;
  ulong length;
  ha_rows start_records;
  my_off_t new_header_length,del;
  File new_file;
  MI_SORT_PARAM sort_param;
  MYISAM_SHARE *share=info->s;
  HA_KEYSEG *keyseg;
  ulong   *rec_per_key_part;
  char llbuff[22];
  SORT_INFO sort_info;
  ulonglong UNINIT_VAR(key_map);
  DBUG_ENTER("mi_repair_by_sort");

  start_records=info->state->records;
  got_error=1;
  new_file= -1;
  new_header_length=(param->testflag & T_UNPACK) ? 0 :
    share->pack.header_length;
  if (!(param->testflag & T_SILENT))
  {
    printf("- recovering (with sort) MyISAM-table '%s'\n",name);
    printf("Data records: %s\n", llstr(start_records,llbuff));
  }
  param->testflag|=T_REP; /* for easy checking */

  if (info->s->options & (HA_OPTION_CHECKSUM | HA_OPTION_COMPRESS_RECORD))
    param->testflag|=T_CALC_CHECKSUM;

  bzero((char*)&sort_info,sizeof(sort_info));
  bzero((char *)&sort_param, sizeof(sort_param));
  if (!(sort_info.key_block=
	alloc_key_blocks(param,
			 (uint) param->sort_key_blocks,
			 share->base.max_key_block_length))
      || init_io_cache(&param->read_cache,info->dfile,
		       (uint) param->read_buffer_length,
		       READ_CACHE,share->pack.header_length,1,MYF(MY_WME)) ||
      (! rep_quick &&
       init_io_cache(&info->rec_cache,info->dfile,
		     (uint) param->write_buffer_length,
		     WRITE_CACHE,new_header_length,1,
		     MYF(MY_WME | MY_WAIT_IF_FULL) & param->myf_rw)))
    goto err;
  sort_info.key_block_end=sort_info.key_block+param->sort_key_blocks;
  info->opt_flag|=WRITE_CACHE_USED;
  info->rec_cache.file=info->dfile;		/* for sort_delete_record */

  if (!mi_alloc_rec_buff(info, -1, &sort_param.record) ||
      !mi_alloc_rec_buff(info, -1, &sort_param.rec_buff))
  {
    mi_check_print_error(param, "Not enough memory for extra record");
    goto err;
  }
  if (!rep_quick)
  {
    /* Get real path for data file */
    if ((new_file= mysql_file_create(mi_key_file_datatmp,
                                     fn_format(param->temp_filename,
                                               share->data_file_name, "",
                                               DATA_TMP_EXT, 2+4),
                                     0, param->tmpfile_createflag,
                                     MYF(0))) < 0)
    {
      mi_check_print_error(param,"Can't create new tempfile: '%s'",
			   param->temp_filename);
      goto err;
    }
    if (new_header_length &&
        filecopy(param, new_file,info->dfile,0L,new_header_length,
		 "datafile-header"))
      goto err;
    if (param->testflag & T_UNPACK)
    {
      share->options&= ~HA_OPTION_COMPRESS_RECORD;
      mi_int2store(share->state.header.options,share->options);
    }
    share->state.dellink= HA_OFFSET_ERROR;
    info->rec_cache.file=new_file;
  }

  info->update= (short) (HA_STATE_CHANGED | HA_STATE_ROW_CHANGED);

  /* Optionally drop indexes and optionally modify the key_map. */
  mi_drop_all_indexes(param, info, FALSE);
  key_map= share->state.key_map;
  if (param->testflag & T_CREATE_MISSING_KEYS)
  {
    /* Invert the copied key_map to recreate all disabled indexes. */
    key_map= ~key_map;
  }

  sort_info.info=info;
  sort_info.param = param;

  set_data_file_type(&sort_info, share);
  sort_param.filepos=new_header_length;
  sort_info.dupp=0;
  sort_info.buff=0;
  param->read_cache.end_of_file=sort_info.filelength=
    mysql_file_seek(param->read_cache.file, 0L, MY_SEEK_END, MYF(0));

  sort_param.wordlist=NULL;
  init_alloc_root(&sort_param.wordroot, FTPARSER_MEMROOT_ALLOC_SIZE, 0);

  if (share->data_file_type == DYNAMIC_RECORD)
    length=max(share->base.min_pack_length+1,share->base.min_block_length);
  else if (share->data_file_type == COMPRESSED_RECORD)
    length=share->base.min_block_length;
  else
    length=share->base.pack_reclength;
  sort_info.max_records=
    ((param->testflag & T_CREATE_MISSING_KEYS) ? info->state->records :
     (ha_rows) (sort_info.filelength/length+1));
  sort_param.key_cmp=sort_key_cmp;
  sort_param.lock_in_memory=lock_memory;
  sort_param.tmpdir=param->tmpdir;
  sort_param.sort_info=&sort_info;
  sort_param.fix_datafile= (my_bool) (! rep_quick);
  sort_param.master =1;
  
  del=info->state->del;
  param->glob_crc=0;
  if (param->testflag & T_CALC_CHECKSUM)
    sort_param.calc_checksum= 1;

  rec_per_key_part= param->rec_per_key_part;
  for (sort_param.key=0 ; sort_param.key < share->base.keys ;
       rec_per_key_part+=sort_param.keyinfo->keysegs, sort_param.key++)
  {
    sort_param.read_cache=param->read_cache;
    sort_param.keyinfo=share->keyinfo+sort_param.key;
    sort_param.seg=sort_param.keyinfo->seg;
    /*
      Skip this index if it is marked disabled in the copied
      (and possibly inverted) key_map.
    */
    if (! mi_is_key_active(key_map, sort_param.key))
    {
      /* Remember old statistics for key */
      memcpy((char*) rec_per_key_part,
	     (char*) (share->state.rec_per_key_part +
		      (uint) (rec_per_key_part - param->rec_per_key_part)),
	     sort_param.keyinfo->keysegs*sizeof(*rec_per_key_part));
      DBUG_PRINT("repair", ("skipping seemingly disabled index #: %u",
                            sort_param.key));
      continue;
    }

    if ((!(param->testflag & T_SILENT)))
      printf ("- Fixing index %d\n",sort_param.key+1);
    sort_param.max_pos=sort_param.pos=share->pack.header_length;
    keyseg=sort_param.seg;
    bzero((char*) sort_param.unique,sizeof(sort_param.unique));
    sort_param.key_length=share->rec_reflength;
    for (i=0 ; keyseg[i].type != HA_KEYTYPE_END; i++)
    {
      sort_param.key_length+=keyseg[i].length;
      if (keyseg[i].flag & HA_SPACE_PACK)
	sort_param.key_length+=get_pack_length(keyseg[i].length);
      if (keyseg[i].flag & (HA_BLOB_PART | HA_VAR_LENGTH_PART))
	sort_param.key_length+=2 + test(keyseg[i].length >= 127);
      if (keyseg[i].flag & HA_NULL_PART)
	sort_param.key_length++;
    }
    info->state->records=info->state->del=share->state.split=0;
    info->state->empty=0;

    if (sort_param.keyinfo->flag & HA_FULLTEXT)
    {
      uint ft_max_word_len_for_sort=FT_MAX_WORD_LEN_FOR_SORT*
                                    sort_param.keyinfo->seg->charset->mbmaxlen;
      sort_param.key_length+=ft_max_word_len_for_sort-HA_FT_MAXBYTELEN;
      /*
        fulltext indexes may have much more entries than the
        number of rows in the table. We estimate the number here.
      */
      if (sort_param.keyinfo->parser == &ft_default_parser)
      {
        /*
          for built-in parser the number of generated index entries
          cannot be larger than the size of the data file divided
          by the minimal word's length
        */
        sort_info.max_records=
          (ha_rows) (sort_info.filelength/ft_min_word_len+1);
      }
      else
      {
        /*
          for external plugin parser we cannot tell anything at all :(
          so, we'll use all the sort memory and start from ~10 buffpeks.
          (see _create_index_by_sort)
        */
        sort_info.max_records= 10 *
                               max(param->sort_buffer_length, MIN_SORT_BUFFER) /
                               sort_param.key_length;
      }

      sort_param.key_read=sort_ft_key_read;
      sort_param.key_write=sort_ft_key_write;
    }
    else
    {
      sort_param.key_read=sort_key_read;
      sort_param.key_write=sort_key_write;
    }

    if (_create_index_by_sort(&sort_param,
			      (my_bool) (!(param->testflag & T_VERBOSE)),
                              param->sort_buffer_length))
    {
      param->retry_repair=1;
      goto err;
    }
    /* No need to calculate checksum again. */
    sort_param.calc_checksum= 0;
    free_root(&sort_param.wordroot, MYF(0));

    /* Set for next loop */
    sort_info.max_records= (ha_rows) info->state->records;

    if (param->testflag & T_STATISTICS)
      update_key_parts(sort_param.keyinfo, rec_per_key_part, sort_param.unique,
                       param->stats_method == MI_STATS_METHOD_IGNORE_NULLS?
                       sort_param.notnull: NULL,
                       (ulonglong) info->state->records);
    /* Enable this index in the permanent (not the copied) key_map. */
    mi_set_key_active(share->state.key_map, sort_param.key);
    DBUG_PRINT("repair", ("set enabled index #: %u", sort_param.key));

    if (sort_param.fix_datafile)
    {
      param->read_cache.end_of_file=sort_param.filepos;
      if (write_data_suffix(&sort_info,1) || end_io_cache(&info->rec_cache))
	goto err;
      if (param->testflag & T_SAFE_REPAIR)
      {
	/* Don't repair if we loosed more than one row */
	if (info->state->records+1 < start_records)
	{
	  info->state->records=start_records;
	  goto err;
	}
      }
      share->state.state.data_file_length = info->state->data_file_length=
	sort_param.filepos;
      /* Only whole records */
      share->state.version=(ulong) time((time_t*) 0);
      mysql_file_close(info->dfile, MYF(0));
      info->dfile=new_file;
      share->data_file_type=sort_info.new_data_file_type;
      share->pack.header_length=(ulong) new_header_length;
      sort_param.fix_datafile=0;
    }
    else
      info->state->data_file_length=sort_param.max_pos;

    param->read_cache.file=info->dfile;		/* re-init read cache */
    reinit_io_cache(&param->read_cache,READ_CACHE,share->pack.header_length,
                    1,1);
  }

  if (param->testflag & T_WRITE_LOOP)
  {
    (void) fputs("          \r",stdout); (void) fflush(stdout);
  }

  if (rep_quick && del+sort_info.dupp != info->state->del)
  {
    mi_check_print_error(param,"Couldn't fix table with quick recovery: Found wrong number of deleted records");
    mi_check_print_error(param,"Run recovery again without -q");
    got_error=1;
    param->retry_repair=1;
    param->testflag|=T_RETRY_WITHOUT_QUICK;
    goto err;
  }

  if (rep_quick & T_FORCE_UNIQUENESS)
  {
    my_off_t skr=info->state->data_file_length+
      (share->options & HA_OPTION_COMPRESS_RECORD ?
       MEMMAP_EXTRA_MARGIN : 0);
#ifdef USE_RELOC
    if (share->data_file_type == STATIC_RECORD &&
	skr < share->base.reloc*share->base.min_pack_length)
      skr=share->base.reloc*share->base.min_pack_length;
#endif
    if (skr != sort_info.filelength)
      if (mysql_file_chsize(info->dfile, skr, 0, MYF(0)))
	mi_check_print_warning(param,
			       "Can't change size of datafile,  error: %d",
			       my_errno);
  }
  if (param->testflag & T_CALC_CHECKSUM)
    info->state->checksum=param->glob_crc;

  if (mysql_file_chsize(share->kfile, info->state->key_file_length, 0, MYF(0)))
    mi_check_print_warning(param,
			   "Can't change size of indexfile, error: %d",
			   my_errno);

  if (!(param->testflag & T_SILENT))
  {
    if (start_records != info->state->records)
      printf("Data records: %s\n", llstr(info->state->records,llbuff));
    if (sort_info.dupp)
      mi_check_print_warning(param,
			     "%s records have been removed",
			     llstr(sort_info.dupp,llbuff));
  }
  got_error=0;

  if (&share->state.state != info->state)
    memcpy( &share->state.state, info->state, sizeof(*info->state));

err:
  got_error|= flush_blocks(param, share->key_cache, share->kfile);
  (void) end_io_cache(&info->rec_cache);
  if (!got_error)
  {
    /* Replace the actual file with the temporary file */
    if (new_file >= 0)
    {
      mysql_file_close(new_file, MYF(0));
      info->dfile=new_file= -1;
      if (change_to_newfile(share->data_file_name,MI_NAME_DEXT, DATA_TMP_EXT,
			    (param->testflag & T_BACKUP_DATA ?
			     MYF(MY_REDEL_MAKE_BACKUP): MYF(0))) ||
	  mi_open_datafile(info,share,name,-1))
	got_error=1;
    }
  }
  if (got_error)
  {
    if (! param->error_printed)
      mi_check_print_error(param,"%d when fixing table",my_errno);
    if (new_file >= 0)
    {
      (void) mysql_file_close(new_file, MYF(0));
      (void) mysql_file_delete(mi_key_file_datatmp,
                               param->temp_filename, MYF(MY_WME));
      if (info->dfile == new_file) /* Retry with key cache */
        if (unlikely(mi_open_datafile(info, share, name, -1)))
          param->retry_repair= 0; /* Safety */
    }
    mi_mark_crashed_on_repair(info);
  }
  else if (key_map == share->state.key_map)
    share->state.changed&= ~STATE_NOT_OPTIMIZED_KEYS;
  share->state.changed|=STATE_NOT_SORTED_PAGES;

  my_free(mi_get_rec_buff_ptr(info, sort_param.rec_buff));
  my_free(mi_get_rec_buff_ptr(info, sort_param.record));
  my_free(sort_info.key_block);
  my_free(sort_info.ft_buf);
  my_free(sort_info.buff);
  (void) end_io_cache(&param->read_cache);
  info->opt_flag&= ~(READ_CACHE_USED | WRITE_CACHE_USED);
  if (!got_error && (param->testflag & T_UNPACK))
  {
    share->state.header.options[0]&= (uchar) ~HA_OPTION_COMPRESS_RECORD;
    share->pack.header_length=0;
  }
  DBUG_RETURN(got_error);
}