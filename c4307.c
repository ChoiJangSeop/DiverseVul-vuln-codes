int mi_repair(MI_CHECK *param, register MI_INFO *info,
	      char * name, int rep_quick)
{
  int error,got_error;
  ha_rows start_records,new_header_length;
  my_off_t del;
  File new_file;
  MYISAM_SHARE *share=info->s;
  char llbuff[22],llbuff2[22];
  SORT_INFO sort_info;
  MI_SORT_PARAM sort_param;
  DBUG_ENTER("mi_repair");

  bzero((char *)&sort_info, sizeof(sort_info));
  bzero((char *)&sort_param, sizeof(sort_param));
  start_records=info->state->records;
  new_header_length=(param->testflag & T_UNPACK) ? 0L :
    share->pack.header_length;
  got_error=1;
  new_file= -1;
  sort_param.sort_info=&sort_info;

  if (!(param->testflag & T_SILENT))
  {
    printf("- recovering (with keycache) MyISAM-table '%s'\n",name);
    printf("Data records: %s\n", llstr(info->state->records,llbuff));
  }
  param->testflag|=T_REP; /* for easy checking */

  if (info->s->options & (HA_OPTION_CHECKSUM | HA_OPTION_COMPRESS_RECORD))
    param->testflag|=T_CALC_CHECKSUM;

  DBUG_ASSERT(param->use_buffers < SIZE_T_MAX);

  if (!param->using_global_keycache)
    (void) init_key_cache(dflt_key_cache, param->key_cache_block_size,
                        param->use_buffers, 0, 0);

  if (init_io_cache(&param->read_cache,info->dfile,
		    (uint) param->read_buffer_length,
		    READ_CACHE,share->pack.header_length,1,MYF(MY_WME)))
  {
    bzero(&info->rec_cache,sizeof(info->rec_cache));
    goto err;
  }
  if (!rep_quick)
    if (init_io_cache(&info->rec_cache,-1,(uint) param->write_buffer_length,
		      WRITE_CACHE, new_header_length, 1,
		      MYF(MY_WME | MY_WAIT_IF_FULL)))
      goto err;
  info->opt_flag|=WRITE_CACHE_USED;
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
        filecopy(param,new_file,info->dfile,0L,new_header_length,
		 "datafile-header"))
      goto err;
    info->s->state.dellink= HA_OFFSET_ERROR;
    info->rec_cache.file=new_file;
    if (param->testflag & T_UNPACK)
    {
      share->options&= ~HA_OPTION_COMPRESS_RECORD;
      mi_int2store(share->state.header.options,share->options);
    }
  }
  sort_info.info=info;
  sort_info.param = param;
  sort_param.read_cache=param->read_cache;
  sort_param.pos=sort_param.max_pos=share->pack.header_length;
  sort_param.filepos=new_header_length;
  param->read_cache.end_of_file=sort_info.filelength=
    mysql_file_seek(info->dfile, 0L, MY_SEEK_END, MYF(0));
  sort_info.dupp=0;
  sort_param.fix_datafile= (my_bool) (! rep_quick);
  sort_param.master=1;
  sort_info.max_records= ~(ha_rows) 0;

  set_data_file_type(&sort_info, share);
  del=info->state->del;
  info->state->records=info->state->del=share->state.split=0;
  info->state->empty=0;
  param->glob_crc=0;
  if (param->testflag & T_CALC_CHECKSUM)
    sort_param.calc_checksum= 1;

  info->update= (short) (HA_STATE_CHANGED | HA_STATE_ROW_CHANGED);

  /* This function always recreates all enabled indexes. */
  if (param->testflag & T_CREATE_MISSING_KEYS)
    mi_set_all_keys_active(share->state.key_map, share->base.keys);
  mi_drop_all_indexes(param, info, TRUE);

  lock_memory(param);			/* Everything is alloced */

  /* Re-create all keys, which are set in key_map. */
  while (!(error=sort_get_next_record(&sort_param)))
  {
    if (writekeys(&sort_param))
    {
      if (my_errno != HA_ERR_FOUND_DUPP_KEY)
	goto err;
      DBUG_DUMP("record",(uchar*) sort_param.record,share->base.pack_reclength);
      mi_check_print_info(param,"Duplicate key %2d for record at %10s against new record at %10s",
			  info->errkey+1,
			  llstr(sort_param.start_recpos,llbuff),
			  llstr(info->dupp_key_pos,llbuff2));
      if (param->testflag & T_VERBOSE)
      {
	(void) _mi_make_key(info,(uint) info->errkey,info->lastkey,
			  sort_param.record,0L);
	_mi_print_key(stdout,share->keyinfo[info->errkey].seg,info->lastkey,
		      USE_WHOLE_KEY);
      }
      sort_info.dupp++;
      if ((param->testflag & (T_FORCE_UNIQUENESS|T_QUICK)) == T_QUICK)
      {
        param->testflag|=T_RETRY_WITHOUT_QUICK;
	param->error_printed=1;
	goto err;
      }
      continue;
    }
    if (sort_write_record(&sort_param))
      goto err;
  }
  if (error > 0 || write_data_suffix(&sort_info, (my_bool)!rep_quick) ||
      flush_io_cache(&info->rec_cache) || param->read_cache.error < 0)
    goto err;

  if (param->testflag & T_WRITE_LOOP)
  {
    (void) fputs("          \r",stdout); (void) fflush(stdout);
  }
  if (mysql_file_chsize(share->kfile, info->state->key_file_length, 0, MYF(0)))
  {
    mi_check_print_warning(param,
			   "Can't change size of indexfile, error: %d",
			   my_errno);
    goto err;
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
  if (param->testflag & T_SAFE_REPAIR)
  {
    /* Don't repair if we loosed more than one row */
    if (info->state->records+1 < start_records)
    {
      info->state->records=start_records;
      got_error=1;
      goto err;
    }
  }

  if (!rep_quick)
  {
    mysql_file_close(info->dfile, MYF(0));
    info->dfile=new_file;
    info->state->data_file_length=sort_param.filepos;
    share->state.version=(ulong) time((time_t*) 0);	/* Force reopen */
  }
  else
  {
    info->state->data_file_length=sort_param.max_pos;
  }
  if (param->testflag & T_CALC_CHECKSUM)
    info->state->checksum=param->glob_crc;

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
  /* If invoked by external program that uses thr_lock */
  if (&share->state.state != info->state)
    memcpy( &share->state.state, info->state, sizeof(*info->state));

err:
  if (!got_error)
  {
    /* Replace the actual file with the temporary file */
    if (new_file >= 0)
    {
      mysql_file_close(new_file, MYF(0));
      info->dfile=new_file= -1;
      /*
        On Windows, the old data file cannot be deleted if it is either
        open, or memory mapped. Closing the file won't remove the memory
        map implicilty on Windows. We closed the data file, but we keep
        the MyISAM table open. A memory map will be closed on the final
        mi_close() only. So we need to unmap explicitly here. After
        renaming the new file under the hook, we couldn't use the map of
        the old file any more anyway.
      */
      if (info->s->file_map)
      {
        (void) my_munmap((char*) info->s->file_map,
                         (size_t) info->s->mmaped_length);
        info->s->file_map= NULL;
      }
      if (change_to_newfile(share->data_file_name, MI_NAME_DEXT, DATA_TMP_EXT,
			    (param->testflag & T_BACKUP_DATA ?
			     MYF(MY_REDEL_MAKE_BACKUP): MYF(0))) ||
	  mi_open_datafile(info,share,name,-1))
	got_error=1;

      param->retry_repair= 0;
    }
  }
  if (got_error)
  {
    if (! param->error_printed)
      mi_check_print_error(param,"%d for record at pos %s",my_errno,
		  llstr(sort_param.start_recpos,llbuff));
    if (new_file >= 0)
    {
      (void) mysql_file_close(new_file, MYF(0));
      (void) mysql_file_delete(mi_key_file_datatmp,
                               param->temp_filename, MYF(MY_WME));
      info->rec_cache.file=-1; /* don't flush data to new_file, it's closed */
    }
    mi_mark_crashed_on_repair(info);
  }
  my_free(mi_get_rec_buff_ptr(info, sort_param.rec_buff));
  my_free(mi_get_rec_buff_ptr(info, sort_param.record));
  my_free(sort_info.buff);
  (void) end_io_cache(&param->read_cache);
  info->opt_flag&= ~(READ_CACHE_USED | WRITE_CACHE_USED);
  (void) end_io_cache(&info->rec_cache);
  got_error|=flush_blocks(param, share->key_cache, share->kfile);
  if (!got_error && param->testflag & T_UNPACK)
  {
    share->state.header.options[0]&= (uchar) ~HA_OPTION_COMPRESS_RECORD;
    share->pack.header_length=0;
    share->data_file_type=sort_info.new_data_file_type;
  }
  share->state.changed|= (STATE_NOT_OPTIMIZED_KEYS | STATE_NOT_SORTED_PAGES |
			  STATE_NOT_ANALYZED);
  DBUG_RETURN(got_error);
}